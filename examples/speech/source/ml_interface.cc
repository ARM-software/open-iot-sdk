
/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ml_interface.h"

#include "AsrClassifier.hpp"
#include "AsrResult.hpp"
#include "AudioUtils.hpp"
#include "BufAttributes.hpp"
#include "Classifier.hpp"
#include "Labels.hpp"
#include "OutputDecode.hpp"
#include "TensorFlowLiteMicro.hpp"
#include "UseCaseCommonUtils.hpp"
#include "Wav2LetterMfcc.hpp"
#include "Wav2LetterModel.hpp"
#include "Wav2LetterPostprocess.hpp"
#include "Wav2LetterPreprocess.hpp"
#include "bsp_serial.h"
#include "cmsis.h"
#include "cmsis_os2.h"
#include "device_mps3.h" /* FPGA level definitions and functions. */
#include "dsp_interfaces.h"
#include "ethos-u55.h"     /* Mem map and configuration definitions of the Ethos U55 */
#include "ethosu_driver.h" /* Arm Ethos-U55 driver header */
#include "hal.h"
#include "model_config.h"
#include "smm_mps3.h"       /* Mem map for MPS3 peripherals. */
#include "timer_mps3.h"     /* Timer functions. */
#include "timing_adapter.h" /* Driver header of the timing adapter */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdbool.h>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include "hal-toolbox/critical_section_api.h"
}

#include "audio_config.h"

// Define tensor arena and declare functions required to access the model
namespace arm {
namespace app {
uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;
namespace asr {
extern uint8_t *GetModelPointer();
extern size_t GetModelLen();
} /* namespace asr */
} /* namespace app */
} /* namespace arm */
typedef std::string ml_processing_state_t;

namespace {

typedef enum { ML_EVENT_START, ML_EVENT_STOP } ml_event_t;

typedef struct {
    ml_event_t event;
} ml_msg_t;

typedef struct {
    char *result;
} ml_mqtt_msg_t;

// Import
using KwsClassifier = arm::app::Classifier;
using namespace arm::app;

// Processing state
static osMessageQueueId_t ml_msg_queue = NULL;
static osMessageQueueId_t ml_mqtt_msg_queue = NULL;
osMutexId_t ml_mutex = NULL;

extern "C" {
void ml_task_inference_start()
{
    const ml_msg_t msg = {ML_EVENT_START};
    if (osMessageQueuePut(ml_msg_queue, (void *)&msg, 0, 0) != osOK) {
        printf_err("Failed to send message to ml_msg_queue\r\n");
    }
}

void ml_task_inference_stop()
{
    const ml_msg_t msg = {ML_EVENT_STOP};
    if (osMessageQueuePut(ml_msg_queue, (void *)&msg, 0, 0) != osOK) {
        printf_err("Failed to send message to ml_msg_queue\r\n");
    }
}
} // extern "C" {

extern "C" void mqtt_send_inference_result(const char *message);

static bool ml_lock()
{
    bool success = false;
    if (ml_mutex) {
        osStatus_t status = osMutexAcquire(ml_mutex, osWaitForever);
        if (status != osOK) {
            printf_err("osMutexAcquire ml_mutex failed %d\r\n", status);
        } else {
            success = true;
        }
    }
    return success;
}

static bool ml_unlock()
{
    bool success = false;
    if (ml_mutex) {
        osStatus_t status = osMutexRelease(ml_mutex);
        if (status != osOK) {
            printf_err("osMutexRelease ml_mutex failed %d\r\n", status);
        } else {
            success = true;
        }
    }
    return success;
}

void send_ml_processing_result(const char *result)
{
    if (!ml_lock()) {
        return;
    }

    size_t msg_len = strlen(result) + 1;
    char *msg_result = reinterpret_cast<char *>(malloc(msg_len));
    if (msg_result) {
        memcpy(msg_result, result, msg_len);
        const ml_mqtt_msg_t msg = {msg_result};
        if (osMessageQueuePut(ml_mqtt_msg_queue, (void *)&msg, 0, 0) != osOK) {
            printf_err("Failed to send message to ml_mqtt_msg_queue\r\n");
            free(reinterpret_cast<void *>(msg_result));
        }
    } else {
        warn("Failed to send ml processing result (alloc failure)");
    }

    ml_unlock();
}

// Model
arm::app::ApplicationContext caseContext;

/**
 * @brief           Presents inference results using the data presentation
 *                  object.
 * @param[in]       platform    Reference to the hal platform object.
 * @param[in]       results     Vector of classification results to be displayed.
 * @return          true if successful, false otherwise.
 **/
static bool PresentInferenceResult(const std::vector<arm::app::asr::AsrResult> &results);

extern "C" {
int ml_frame_length()
{
    const auto frameLength = caseContext.Get<int>("frameLength"); // 640
    return (frameLength);
}

int ml_frame_stride()
{
    const auto frameStride = caseContext.Get<int>("frameStride"); // 320
    return (frameStride);
}
}

void ProcessAudio(ApplicationContext &ctx, DSPML *dspMLConnection)
{
    /* Get model reference. */
    auto &model = ctx.Get<Model &>("model");
    if (!model.IsInited()) {
        printf_err("Model is not initialised! Terminating processing.\n");
        return;
    }

    /* Get score threshold to be applied for the classifier (post-inference). */
    auto scoreThreshold = ctx.Get<float>("scoreThreshold");

    /* Get tensors. Dimensions of the tensor should have been verified by
     * the callee. */
    TfLiteTensor *inputTensor = model.GetInputTensor(0);
    TfLiteTensor *outputTensor = model.GetOutputTensor(0);
    TfLiteIntArray *inputShape = model.GetInputShape(0);

    /* Populate MFCC related parameters. */
    auto mfccFrameLen = ctx.Get<uint32_t>("frameLength");
    auto mfccFrameStride = ctx.Get<uint32_t>("frameStride");

    /* Populate ASR inference context and inner lengths for input. */
    auto inputCtxLen = ctx.Get<uint32_t>("ctxLen");

    /* Get pre/post-processing objects. */
    AsrPreProcess preProcess = AsrPreProcess(inputTensor,
                                             Wav2LetterModel::ms_numMfccFeatures,
                                             inputShape->data[Wav2LetterModel::ms_inputRowsIdx],
                                             mfccFrameLen,
                                             mfccFrameStride);

    std::vector<ClassificationResult> singleInfResult;
    const uint32_t outputCtxLen = AsrPostProcess::GetOutputContextLen(model, inputCtxLen);
    AsrPostProcess postProcess = AsrPostProcess(outputTensor,
                                                ctx.Get<AsrClassifier &>("classifier"),
                                                ctx.Get<std::vector<std::string> &>("labels"),
                                                singleInfResult,
                                                outputCtxLen,
                                                Wav2LetterModel::ms_blankTokenIdx,
                                                Wav2LetterModel::ms_outputRowsIdx);

    const uint32_t inputRows = inputTensor->dims->data[arm::app::Wav2LetterModel::ms_inputRowsIdx];
    /* Audio data stride corresponds to inputInnerLen feature vectors. */
    const uint32_t audioParamsWinLen = inputRows * mfccFrameStride;

    auto inferenceWindow = std::vector<int16_t>(audioParamsWinLen, 0);
    size_t inferenceWindowLen = audioParamsWinLen;

    // Start processing audio data as it arrive
    ml_msg_t msg;
    uint32_t inferenceIndex = 0;
    // We do inference on 2 audio segments before reporting a result
    // We do not have the concept of audio clip in a streaming application
    // so we need to decide when a sentenced is finished to start a recognition.
    // It was arbitrarily chosen to be 2 inferences.
    // In a real app, a voice activity detector would probably be used
    // to detect a long silence between 2 sentences.
    const uint32_t maxNbInference = 2;
    std::vector<arm::app::asr::AsrResult> results;

    while (true) {
        while (true) {
            if (osMessageQueueGet(ml_msg_queue, &msg, NULL, 0) == osOK) {
                if (msg.event == ML_EVENT_STOP) {
                    /* jump out to outer loop */
                    break;
                } /* else it's ML_EVENT_START so we fall through and continue with the code */
            }

            // Wait for the DSP task signal to start the recognition
            dspMLConnection->waitForDSPData();

            int16_t *p = inferenceWindow.data();
            dspMLConnection->copyFromMLBufferInto(p);

            // This timestamp is corresponding to the time when
            // inference is starting and not to the time of the
            // beginning of the audio segment used for this inference.
            float currentTimeStamp = get_audio_timestamp();
            info("Inference %i/%i\n", inferenceIndex + 1, maxNbInference);

            /* Run the pre-processing, inference and post-processing. */
            if (!preProcess.DoPreProcess(inferenceWindow.data(), inferenceWindowLen)) {
                printf_err("Pre-processing failed.");
            }

            info("Start running inference\n");

            /* Run inference over this audio clip sliding window. */
            if (!model.RunInference()) {
                printf_err("Failed to run inference");
                return;
            }

            info("Doing post processing\n");

            /* Post processing needs to know if we are on the last audio window. */
            // postProcess.m_lastIteration = !audioDataSlider.HasNext();
            if (!postProcess.DoPostProcess()) {
                printf_err("Post-processing failed.");
            }

            info("Inference done\n");

            std::vector<ClassificationResult> classificationResult;
            auto &classifier = ctx.Get<AsrClassifier &>("classifier");
            classifier.GetClassificationResults(
                outputTensor, classificationResult, ctx.Get<std::vector<std::string> &>("labels"), 1, true);

            auto result = asr::AsrResult(classificationResult, currentTimeStamp, inferenceIndex, scoreThreshold);

            results.emplace_back(result);

            inferenceIndex = inferenceIndex + 1;
            if (inferenceIndex == maxNbInference) {

                inferenceIndex = 0;

                ctx.Set<std::vector<arm::app::asr::AsrResult>>("results", results);

                if (!PresentInferenceResult(results)) {
                    return;
                }

                results.clear();
            }

            // Inference loop
        } /* while (true) */

        while (osMessageQueueGet(ml_msg_queue, &msg, NULL, osWaitForever) == osOK) {
            if (msg.event == ML_EVENT_START) {
                break;
            } /* else it's ML_EVENT_STOP so we keep waiting */
        }
    } /* while (true) */
}

static bool PresentInferenceResult(const std::vector<arm::app::asr::AsrResult> &results)
{
    info("Final results:\n");
    info("Total number of inferences: %zu\n", results.size());
    /* Results from multiple inferences should be combined before processing. */
    std::vector<arm::app::ClassificationResult> combinedResults;
    for (auto &result : results) {
        combinedResults.insert(combinedResults.end(), result.m_resultVec.begin(), result.m_resultVec.end());
    }

    /* Get each inference result string using the decoder. */
    for (const auto &result : results) {
        std::string infResultStr = audio::asr::DecodeOutput(result.m_resultVec);

        info("For timestamp: %f (inference #: %" PRIu32 "); label: %s\n",
             (double)result.m_timeStamp,
             result.m_inferenceNumber,
             infResultStr.c_str());
    }

    /* Get the decoded result for the combined result. */
    std::string finalResultStr = audio::asr::DecodeOutput(combinedResults);

    info("Complete recognition: %s\n", finalResultStr.c_str());

    // Send the inference result
    send_ml_processing_result(finalResultStr.c_str());

    return true;
}

} // anonymous namespace

extern struct ethosu_driver ethosu_drv; /* Default Ethos-U55 device driver */

/**
 * @brief   Initialises the Arm Ethos-U55 NPU
 * @return  0 if successful, error code otherwise
 **/
static int arm_npu_init(void);

/**
 * @brief   Defines the Ethos-U interrupt handler: just a wrapper around the default
 *          implementation.
 **/
extern "C" {
void arm_npu_irq_handler(void)
{
    /* Call the default interrupt handler from the NPU driver */
    ethosu_irq_handler(&ethosu_drv);
}
}

/**
 * @brief  Initialises the NPU IRQ
 **/
static void arm_npu_irq_init(void)
{
    const IRQn_Type ethosu_irqnum = (IRQn_Type)EthosU_IRQn;

    /* Register the EthosU IRQ handler in our vector table.
     * Note, this handler comes from the EthosU driver */
    NVIC_SetVector(ethosu_irqnum, (uint32_t)arm_npu_irq_handler);

    /* Enable the IRQ */
    NVIC_EnableIRQ(ethosu_irqnum);

    debug("EthosU IRQ#: %u, Handler: 0x%p\n", ethosu_irqnum, arm_npu_irq_handler);
}

static int _arm_npu_timing_adapter_init(void)
{
#if defined(TA0_BASE)
    struct timing_adapter ta_0;
    struct timing_adapter_settings ta_0_settings = {.maxr = TA0_MAXR,
                                                    .maxw = TA0_MAXW,
                                                    .maxrw = TA0_MAXRW,
                                                    .rlatency = TA0_RLATENCY,
                                                    .wlatency = TA0_WLATENCY,
                                                    .pulse_on = TA0_PULSE_ON,
                                                    .pulse_off = TA0_PULSE_OFF,
                                                    .bwcap = TA0_BWCAP,
                                                    .perfctrl = TA0_PERFCTRL,
                                                    .perfcnt = TA0_PERFCNT,
                                                    .mode = TA0_MODE,
                                                    .maxpending = 0, /* This is a read-only parameter */
                                                    .histbin = TA0_HISTBIN,
                                                    .histcnt = TA0_HISTCNT};

    if (0 != ta_init(&ta_0, TA0_BASE)) {
        printf_err("TA0 initialisation failed\n");
        return 1;
    }

    ta_set_all(&ta_0, &ta_0_settings);
#endif /* defined (TA0_BASE) */

#if defined(TA1_BASE)
    struct timing_adapter ta_1;
    struct timing_adapter_settings ta_1_settings = {.maxr = TA1_MAXR,
                                                    .maxw = TA1_MAXW,
                                                    .maxrw = TA1_MAXRW,
                                                    .rlatency = TA1_RLATENCY,
                                                    .wlatency = TA1_WLATENCY,
                                                    .pulse_on = TA1_PULSE_ON,
                                                    .pulse_off = TA1_PULSE_OFF,
                                                    .bwcap = TA1_BWCAP,
                                                    .perfctrl = TA1_PERFCTRL,
                                                    .perfcnt = TA1_PERFCNT,
                                                    .mode = TA1_MODE,
                                                    .maxpending = 0, /* This is a read-only parameter */
                                                    .histbin = TA1_HISTBIN,
                                                    .histcnt = TA1_HISTCNT};

    if (0 != ta_init(&ta_1, TA1_BASE)) {
        printf_err("TA1 initialisation failed\n");
        return 1;
    }

    ta_set_all(&ta_1, &ta_1_settings);
#endif /* defined (TA1_BASE) */

    return 0;
}

static int arm_npu_init(void)
{
    int err = 0;

    /* If the platform has timing adapter blocks along with Ethos-U55 core
     * block, initialise them here. */
    // cppcheck-suppress knownConditionTrueFalse
    if (0 != (err = _arm_npu_timing_adapter_init())) {
        return err;
    }

    /* Initialise the IRQ */
    arm_npu_irq_init();

    /* Initialise Ethos-U55 device */
    const void *ethosu_base_address = reinterpret_cast<const void *>(SEC_ETHOS_U55_BASE);

    if (0
        != (err = ethosu_init(&ethosu_drv,         /* Ethos-U55 driver device pointer */
                              ethosu_base_address, /* Ethos-U55's base address. */
                              NULL,                /* Pointer to fast mem area - NULL for U55. */
                              0,                   /* Fast mem region size. */
                              0,                   /* Security enable. */
                              0))) {               /* Privilege enable. */
        printf_err("failed to initalise Ethos-U55 device\n");
        return err;
    }

    info("Ethos-U55 device initialised\n");

    /* Get Ethos-U55 version */
    struct ethosu_driver_version driver_version;
    struct ethosu_hw_info hw_info;

    ethosu_get_driver_version(&driver_version);
    ethosu_get_hw_info(&ethosu_drv, &hw_info);

    info("Ethos-U version info:\n");
    info("\tArch:       v%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
         hw_info.version.arch_major_rev,
         hw_info.version.arch_minor_rev,
         hw_info.version.arch_patch_rev);
    info("\tDriver:     v%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n",
         driver_version.major,
         driver_version.minor,
         driver_version.patch);
    info("\tMACs/cc:    %" PRIu32 "\n", (uint32_t)(1 << hw_info.cfg.macs_per_cc));
    info("\tCmd stream: v%" PRIu32 "\n", hw_info.cfg.cmd_stream_version);

    return 0;
}

extern "C" {
int ml_interface_init()
{
    static arm::app::Wav2LetterModel model;    /* Model wrapper object. */
    static arm::app::AsrClassifier classifier; /* Classifier wrapper object. */
    static std::vector<std::string> labels;

    // Initialize the ethos U55
    if (arm_npu_init() != 0) {
        printf_err("Failed to arm npu\n");
        return -1;
    }

    /* Load the model. */
    if (!model.Init(::arm::app::tensorArena,
                    sizeof(::arm::app::tensorArena),
                    ::arm::app::asr::GetModelPointer(),
                    ::arm::app::asr::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return -1;
    }

    /* Initialise post-processing. */
    GetLabelsVector(labels);

    /* Instantiate application context. */
    caseContext.Set<arm::app::Model &>("model", model);
    caseContext.Set<uint32_t>("frameLength", g_FrameLength);
    caseContext.Set<uint32_t>("frameStride", g_FrameStride);
    caseContext.Set<uint32_t>("ctxLen", g_ctxLen);

    caseContext.Set<float>("scoreThreshold", g_ScoreThreshold); /* Normalised score threshold. */

    caseContext.Set<const std::vector<std::string> &>("labels", labels);
    caseContext.Set<arm::app::AsrClassifier &>("classifier", classifier);

    PrintTensorFlowVersion();
    printf("*** ML interface initialised\r\n");
    return 0;
}

void ml_task(void *pvParameters)
{
    DSPML *dspMLConnection = static_cast<DSPML *>(pvParameters);

    ml_mutex = osMutexNew(NULL);
    if (!ml_mutex) {
        printf_err("Failed to create ml_mutex\r\n");
        return;
    }

    ml_msg_queue = osMessageQueueNew(10, sizeof(ml_msg_t), NULL);
    if (!ml_msg_queue) {
        printf_err("Failed to create ml msg queue\r\n");
        return;
    }

    while (1) {
        ml_msg_t msg;
        if (osMessageQueueGet(ml_msg_queue, &msg, NULL, osWaitForever) == osOK) {
            if (msg.event == ML_EVENT_START) {
                break;
            } /* else it's ML_EVENT_STOP so we keep waiting the loop */
        } else {
            printf_err("osMessageQueueGet ml msg queue failed\r\n");
            return;
        }
    }

    if (ml_interface_init() < 0) {
        printf_err("ml_interface_init failed\r\n");
        return;
    }

    ProcessAudio(caseContext, dspMLConnection);
}

void ml_mqtt_task(void *arg)
{
    (void)arg;

    ml_mqtt_msg_queue = osMessageQueueNew(2, sizeof(ml_mqtt_msg_t), NULL);
    if (!ml_mqtt_msg_queue) {
        printf_err("Failed to create a ml mqtt msg queue\r\n");
        return;
    }

    while (1) {
        ml_mqtt_msg_t msg;
        if (osMessageQueueGet(ml_mqtt_msg_queue, &msg, NULL, osWaitForever) == osOK) {
            mqtt_send_inference_result(msg.result);
            free(reinterpret_cast<void *>(msg.result));
        } else {
            printf_err("osMessageQueueGet ml mqtt msg queue failed\r\n");
            return;
        }
    }
}

} // extern "C"
