
/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <array>
#include <stdbool.h>

#include "ml_interface.h"
#include "bsp_serial.h"

#include "Labels.hpp"

#include "AudioUtils.hpp"
#include "Classifier.hpp"
#include "DsCnnMfcc.hpp"
#include "DsCnnModel.hpp"
#include "KwsResult.hpp"
#include "TensorFlowLiteMicro.hpp"
#include "UseCaseCommonUtils.hpp"
#include "audio_processing.h"

#include "ethos-u55.h" /* Mem map and configuration definitions of the Ethos U55 */

#include "smm_mps3.h"    /* Mem map for MPS3 peripherals. */
#include "glcd_mps3.h"   /* LCD functions. */
#include "timer_mps3.h"  /* Timer functions. */
#include "device_mps3.h" /* FPGA level definitions and functions. */

#include "ethosu_driver.h"  /* Arm Ethos-U55 driver header */
#include "timing_adapter.h" /* Driver header of the timing adapter */

#include "hal.h"
#include <vector>
#include <functional>
#include "cmsis_os2.h"

extern "C" {
#include "fvp_sai.h"
#include "hal/sai_api.h"
#include "hal-toolbox/critical_section_api.h"
}

#define AUDIO_BLOCK_NUM   (4)
#define AUDIO_BLOCK_SIZE  (3200)
#define AUDIO_BUFFER_SIZE (AUDIO_BLOCK_NUM * AUDIO_BLOCK_SIZE)

// Convert a pointer in NS DDR memory to its S alias
#define NS2S(T, P) ((T)(((uintptr_t)(P)) + 0x10000000))

namespace {

typedef enum { ML_EVENT_START, ML_EVENT_STOP } ml_event_t;

typedef struct {
    ml_event_t event;
} ml_msg_t;

typedef struct {
    ml_processing_state_t state;
} ml_mqtt_msg_t;

// Import
using KwsClassifier = arm::app::Classifier;
using namespace arm::app;

// audio constants
__attribute__((section(".bss.NoInit.audio_buf"))) __attribute__((aligned(4)))
int16_t shared_audio_buffer[AUDIO_BUFFER_SIZE / 2];
const int kAudioSampleFrequency = 16000;

// Processing state
static osMessageQueueId_t ml_msg_queue = NULL;
static osMessageQueueId_t ml_mqtt_msg_queue = NULL;
osMutexId_t ml_mutex = NULL;
ml_processing_state_t ml_processing_state;
ml_processing_change_handler_t ml_processing_change_handler = NULL;
void *ml_processing_change_ptr = NULL;
const std::array<std::pair<const char *, ml_processing_state_t>, 12> label_to_state{
    std::pair<const char *, ml_processing_state_t>{"_silence_", ML_SILENCE},
    std::pair<const char *, ml_processing_state_t>{"_unknown_", ML_UNKNOWN},
    std::pair<const char *, ml_processing_state_t>{"yes", ML_HEARD_YES},
    std::pair<const char *, ml_processing_state_t>{"no", ML_HEARD_NO},
    std::pair<const char *, ml_processing_state_t>{"up", ML_HEARD_UP},
    std::pair<const char *, ml_processing_state_t>{"down", ML_HEARD_DOWN},
    std::pair<const char *, ml_processing_state_t>{"left", ML_HEARD_LEFT},
    std::pair<const char *, ml_processing_state_t>{"right", ML_HEARD_RIGHT},
    std::pair<const char *, ml_processing_state_t>{"on", ML_HEARD_ON},
    std::pair<const char *, ml_processing_state_t>{"off", ML_HEARD_OFF},
    std::pair<const char *, ml_processing_state_t>{"stop", ML_HEARD_GO},
    std::pair<const char *, ml_processing_state_t>{"go", ML_HEARD_STOP},
};

extern "C" {
const char *get_inference_result_string(ml_processing_state_t ref_state)
{
    return (label_to_state[ref_state].first);
}

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

extern "C" void mqtt_send_inference_result(ml_processing_state_t state);

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

void set_ml_processing_state(ml_processing_state_t new_state)
{
    if (!ml_lock()) {
        return;
    }

    if (new_state != ml_processing_state) {
        // mqtt_send_inference_result(new_state);
        const ml_mqtt_msg_t msg = {new_state};
        if (osMessageQueuePut(ml_mqtt_msg_queue, (void *)&msg, 0, 0) != osOK) {
            printf_err("Failed to send message to ml_mqtt_msg_queue\r\n");
        }

        ml_processing_state = new_state;
        if (ml_processing_change_handler) {
            // Copy the handler data to release the mutex, this allow the external
            // code to not be blocked if it calls the getter or change the handler.
            ml_processing_change_handler_t handler = ml_processing_change_handler;
            void *handler_instance = ml_processing_change_ptr;

            ml_unlock();

            handler(handler_instance, new_state);
            return;
        }
    }

    ml_unlock();
}

// Model
arm::app::ApplicationContext caseContext;

// Audio driver data
void (*event_fn)(void *);
void *event_ptr = nullptr;

// Audio driver configuration & event management
static void AudioEvent(mdh_sai_t *self, void *ctx, mdh_sai_transfer_complete_t code)
{
    if (code == MDH_SAI_TRANSFER_COMPLETE_CANCELLED) {
        printf_err("Transfer cancelled\n");
    }
    if (code == MDH_SAI_TRANSFER_COMPLETE_DONE) {
        if (event_fn) {
            event_fn(event_ptr);
        }
    }
}

static void sai_handler_error(mdh_sai_t *self, mdh_sai_event_t code)
{
    printf_err("Error during SAI transfer\n");
}

int AudioDrv_Setup(void (*event_handler)(void *), void *event_handler_ptr)
{
    fvp_sai_t *fvpsai = fvp_sai_init(1U, 16U, static_cast<uint32_t>(kAudioSampleFrequency), AUDIO_BLOCK_SIZE);

    if (!fvpsai) {
        printf_err("Failed to set up FVP SAI!\n");
        return -1;
    }

    mdh_sai_t *sai = &fvpsai->sai;
    mdh_sai_status_t ret = mdh_sai_set_transfer_complete_callback(sai, AudioEvent);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        printf_err("Failed to set transfer complete callback");
        return ret;
    }

    ret = mdh_sai_set_event_callback(sai, sai_handler_error);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        printf_err("Failed to enable transfer error callback");
        return ret;
    }

    ret = mdh_sai_transfer(sai, (uint8_t *)NS2S(void *, shared_audio_buffer), AUDIO_BLOCK_NUM, NULL);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        printf_err("Failed to start audio transfer");
        return ret;
    }

    event_fn = event_handler;
    event_ptr = event_handler_ptr;

    return 0;
}

/*
 * Access synchronously data from the audio driver.
 *
 * If data is not available, the audio processing thread goes to sleep until it
 * is woken up by the audio driver.
 */
template <typename T> struct CircularSlidingWindow {
    CircularSlidingWindow(
        const T *buffer, size_t block_size, size_t block_count, size_t window_size, size_t stride_size)
        : buffer{buffer}, block_size{block_size}, block_count{block_count}, window_size{window_size}, stride_size{
                                                                                                          stride_size}
    {
        // These are the requirements for the algorithm.
        assert(stride_size < block_size);
        assert(window_size > stride_size);
        assert(block_size > window_size);
        assert(block_size % stride_size == 0);
        assert(window_size % stride_size == 0);
    }

    ~CircularSlidingWindow()
    {
        osSemaphoreDelete(semaphore);
    }

    void next(T *dest)
    {
        // Compute the block that contains the stride
        size_t first_block = current_stride / strides_per_block();
        auto last_block = ((current_stride * stride_size + window_size - 1) / block_size) % block_count;

        // Go to sleep if one of the block that contains the next stride is being written.
        // If the stride is already loaded, copy it into the destination buffer.
        while (first_block == get_block_under_write() || last_block == get_block_under_write()) {
            osStatus_t status = osSemaphoreAcquire(semaphore, osWaitForever);
            if (status != osOK) {
                printf_err("osSemaphoreAcquire failed %d\r\n", status);
            }
        }

        // Copy the data into the destination buffer
        auto begin = buffer + (current_stride * stride_size);

        // Memory to copy may not be seqquential if a window span on two blocks.
        if (last_block < first_block) {
            // Copy end of the buffer
            auto buffer_end = buffer + (block_size * block_count);
            std::copy(begin, buffer_end, dest);
            // Copy remaining from the begining
            auto offset = buffer_end - begin;
            std::copy(buffer, buffer + (window_size - offset), dest + offset);
        } else {
            std::copy(begin, begin + window_size, dest);
        }

        // Compute the next stride
        ++current_stride;
        current_stride %= stride_count();
    }

    // This is called from ISR
    static void signal_block_written(void *ptr)
    {
        auto *self = reinterpret_cast<CircularSlidingWindow<T> *>(ptr);
        // Update block ID
        self->block_under_write = ((self->block_under_write + 1) % self->block_count);

        // Wakeup task waiting
        (void)osSemaphoreRelease(self->semaphore);
        // safe to return error, this can signal multiple times before the reader acquires the semaphore
    }

private:
    size_t stride_count() const
    {
        return ((block_size * block_count) / stride_size);
    }

    size_t strides_per_block() const
    {
        return block_size / stride_size;
    }

    size_t get_block_under_write() const
    {
        hal_critical_section_enter();
        auto result = block_under_write;
        hal_critical_section_exit();
        return result;
    }

    const T *buffer;
    size_t block_size; /* write size */
    size_t block_count;
    size_t window_size;
    size_t stride_size; /* read size, smaller than write size */
    size_t block_under_write = 0;
    size_t current_stride = 0;
    osSemaphoreId_t semaphore = osSemaphoreNew(1U, 1U, NULL);
};

/**
 * @brief           Presents inference results using the data presentation
 *                  object.
 * @param[in]       platform    Reference to the hal platform object.
 * @param[in]       results     Vector of classification results to be displayed.
 * @return          true if successful, false otherwise.
 **/
static void PresentInferenceResult(const arm::app::kws::KwsResult &result);

/**
 * @brief Returns a function to perform feature calculation and populates input tensor data with
 * MFCC data.
 *
 * Input tensor data type check is performed to choose correct MFCC feature data type.
 * If tensor has an integer data type then original features are quantised.
 *
 * Warning: MFCC calculator provided as input must have the same life scope as returned function.
 *
 * @param[in]       mfcc          MFCC feature calculator.
 * @param[in,out]   inputTensor   Input tensor pointer to store calculated features.
 * @param[in]       cacheSize     Size of the feature vectors cache (number of feature vectors).
 * @return          Function to be called providing audio sample and sliding window index.
 */
static std::function<void(std::vector<int16_t> &, int, bool, size_t)>
GetFeatureCalculator(audio::DsCnnMFCC &mfcc, TfLiteTensor *inputTensor, size_t cacheSize);

// Convert labels into ml_processing_state_t
ml_processing_state_t convert_inference_result(const std::string &label)
{
    for (const auto &label_to_state_pair : label_to_state) {
        if (label == label_to_state_pair.first) {
            return label_to_state_pair.second;
        }
    }
    return ML_UNKNOWN;
}

void ProcessAudio(ApplicationContext &ctx)
{
    // Constants
    constexpr int minTensorDims =
        static_cast<int>((arm::app::DsCnnModel::ms_inputRowsIdx > arm::app::DsCnnModel::ms_inputColsIdx)
                             ? arm::app::DsCnnModel::ms_inputRowsIdx
                             : arm::app::DsCnnModel::ms_inputColsIdx);

    // Get the global model
    auto &model = ctx.Get<Model &>("model");

    if (!model.IsInited()) {
        printf_err("Model is not initialised! Terminating processing.\n");
        return;
    }

    const auto frameLength = ctx.Get<int>("frameLength");         // 640
    const auto frameStride = ctx.Get<int>("frameStride");         // 320
    const auto scoreThreshold = ctx.Get<float>("scoreThreshold"); // 0.8

    // Input and output tensors
    TfLiteTensor *outputTensor = model.GetOutputTensor(0);
    TfLiteTensor *inputTensor = model.GetInputTensor(0);

    if (!inputTensor->dims) {
        printf_err("Invalid input tensor dims\n");
        return;
    } else if (inputTensor->dims->size < minTensorDims) {
        printf_err("Input tensor dimension should be >= %d\n", minTensorDims);
        return;
    }

    TfLiteIntArray *inputShape = model.GetInputShape(0);
    const uint32_t kNumCols = inputShape->data[arm::app::DsCnnModel::ms_inputColsIdx];
    const uint32_t kNumRows = inputShape->data[arm::app::DsCnnModel::ms_inputRowsIdx];

    audio::DsCnnMFCC mfcc = audio::DsCnnMFCC(kNumCols, frameLength);
    mfcc.Init();

    /* Deduce the data length required for 1 inference from the network parameters. */
    auto audioDataWindowSize = kNumRows * frameStride + (frameLength - frameStride); // 16000
    auto mfccWindowSize = frameLength;                                               // 640
    auto mfccWindowStride = frameStride;                                             // 320

    /* We choose to move by half the window size => for a 1 second window size
     * there is an overlap of 0.5 seconds. */
    auto audioDataStride = audioDataWindowSize / 2;

    /* To have the previously calculated features re-usable, stride must be multiple
     * of MFCC features window stride. */
    if (0 != audioDataStride % mfccWindowStride) {
        /* Reduce the stride. */
        audioDataStride -= audioDataStride % mfccWindowStride; // 8000
    }

    auto nMfccVectorsInAudioStride = audioDataStride / mfccWindowStride; // 25

    /* We expect to be sampling 1 second worth of data at a time.
     * NOTE: This is only used for time stamp calculation. */
    const float secondsPerSample = 1.0 / audio::DsCnnMFCC::ms_defaultSamplingFreq;

    /* Calculate number of the feature vectors in the window overlap region.
     * These feature vectors will be reused.*/
    auto numberOfReusedFeatureVectors = nMfccVectorsInAudioStride;

    /* Construct feature calculation function. */
    auto mfccFeatureCalc = GetFeatureCalculator(mfcc, inputTensor, numberOfReusedFeatureVectors);

    if (!mfccFeatureCalc) {
        printf_err("No feature calculator available");
        return;
    }

    // Initialize the sliding window
    auto circularSlider = CircularSlidingWindow<int16_t>(
        shared_audio_buffer, AUDIO_BLOCK_SIZE / sizeof(int16_t), AUDIO_BLOCK_NUM, mfccWindowSize, mfccWindowStride);

    // Initialize the audio driver. It is delayed until that point to avoid drop
    // of starting frames.
    AudioDrv_Setup(&decltype(circularSlider)::signal_block_written, &circularSlider);

    bool first_iteration = true;
    auto mfccAudioData = std::vector<int16_t>(mfccWindowSize, 0);
    size_t audio_index = 0;

    // Start processing audio data as it arrive
    ml_msg_t msg;
    while (true) {
        while (true) {
            if (osMessageQueueGet(ml_msg_queue, &msg, NULL, 0) == osOK) {
                if (msg.event == ML_EVENT_STOP) {
                    /* jump out to outer loop */
                    break;
                } /* else it's ML_EVENT_START so we fall through and continue with the code */
            }

            /* The first window does not have cache ready. */
            bool useCache = first_iteration == false && numberOfReusedFeatureVectors > 0;
            size_t stride_index = 0;

            while (stride_index < (audioDataWindowSize / mfccWindowStride)) {
                if (!useCache || stride_index >= numberOfReusedFeatureVectors) {
                    circularSlider.next(mfccAudioData.data());
                }

                /* Compute features for this window and write them to input tensor. */
                mfccFeatureCalc(mfccAudioData, stride_index, useCache, nMfccVectorsInAudioStride);
                ++stride_index;
            }

            /* Run inference over this audio clip sliding window. */
            if (!model.RunInference()) {
                printf_err("Failed to run inference");
                return;
            }

            std::vector<ClassificationResult> classificationResult;
            auto &classifier = ctx.Get<KwsClassifier &>("classifier");
            classifier.GetClassificationResults(
                outputTensor, classificationResult, ctx.Get<std::vector<std::string> &>("labels"), 1);

            auto result = kws::KwsResult(
                classificationResult, audio_index * secondsPerSample * audioDataStride, audio_index, scoreThreshold);

            if (result.m_resultVec.empty()) {
                set_ml_processing_state(ML_UNKNOWN);
            } else {
                set_ml_processing_state(convert_inference_result(result.m_resultVec[0].m_label));
            }

            PresentInferenceResult(result);
            first_iteration = false;
            ++audio_index;
        } /* while (true) */

        while (osMessageQueueGet(ml_msg_queue, &msg, NULL, osWaitForever) == osOK) {
            if (msg.event == ML_EVENT_START) {
                break;
            } /* else it's ML_EVENT_STOP so we keep waiting */
        }
    } /* while (true) */
}

static void PresentInferenceResult(const arm::app::kws::KwsResult &result)
{
    /* Display each result */
    if (result.m_resultVec.empty()) {
        info("For timestamp: %f (inference #: %" PRIu32 "); label: %s; threshold: %f\n",
             result.m_timeStamp,
             result.m_inferenceNumber,
             "<none>",
             0.f);
    } else {
        for (uint32_t i = 0; i < result.m_resultVec.size(); ++i) {
            info("For timestamp: %f (inference #: %" PRIu32 "); label: %s, score: %f; threshold: %f\n",
                 result.m_timeStamp,
                 result.m_inferenceNumber,
                 result.m_resultVec[i].m_label.c_str(),
                 result.m_resultVec[i].m_normalisedVal,
                 result.m_threshold);
        }
    }
}

/**
 * @brief Generic feature calculator factory.
 *
 * Returns lambda function to compute features using features cache.
 * Real features math is done by a lambda function provided as a parameter.
 * Features are written to input tensor memory.
 *
 * @tparam T                Feature vector type.
 * @param[in] inputTensor   Model input tensor pointer.
 * @param[in] cacheSize     Number of feature vectors to cache. Defined by the sliding window overlap.
 * @param[in] compute       Features calculator function.
 * @return                  Lambda function to compute features.
 */
template <class T>
std::function<void(std::vector<int16_t> &, size_t, bool, size_t)>
FeatureCalc(TfLiteTensor *inputTensor, size_t cacheSize, std::function<std::vector<T>(std::vector<int16_t> &)> compute)
{
    /* Feature cache to be captured by lambda function. */
    static std::vector<std::vector<T>> featureCache = std::vector<std::vector<T>>(cacheSize);

    return [=](std::vector<int16_t> &audioDataWindow, size_t index, bool useCache, size_t featuresOverlapIndex) {
        T *tensorData = tflite::GetTensorData<T>(inputTensor);
        std::vector<T> features;

        /* Reuse features from cache if cache is ready and sliding windows overlap.
         * Overlap is in the beginning of sliding window with a size of a feature cache. */
        if (useCache && index < featureCache.size()) {
            features = std::move(featureCache[index]);
        } else {
            features = std::move(compute(audioDataWindow));
        }
        auto size = features.size();
        auto sizeBytes = sizeof(T) * size;
        std::memcpy(tensorData + (index * size), features.data(), sizeBytes);

        /* Start renewing cache as soon iteration goes out of the windows overlap. */
        if (index >= featuresOverlapIndex) {
            featureCache[index - featuresOverlapIndex] = std::move(features);
        }
    };
}

template std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> FeatureCalc<int8_t>(
    TfLiteTensor *inputTensor, size_t cacheSize, std::function<std::vector<int8_t>(std::vector<int16_t> &)> compute);

template std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> FeatureCalc<uint8_t>(
    TfLiteTensor *inputTensor, size_t cacheSize, std::function<std::vector<uint8_t>(std::vector<int16_t> &)> compute);

template std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> FeatureCalc<int16_t>(
    TfLiteTensor *inputTensor, size_t cacheSize, std::function<std::vector<int16_t>(std::vector<int16_t> &)> compute);

template std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> FeatureCalc<float>(
    TfLiteTensor *inputTensor, size_t cacheSize, std::function<std::vector<float>(std::vector<int16_t> &)> compute);

static std::function<void(std::vector<int16_t> &, int, bool, size_t)>
GetFeatureCalculator(audio::DsCnnMFCC &mfcc, TfLiteTensor *inputTensor, size_t cacheSize)
{
    std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> mfccFeatureCalc;
    TfLiteQuantization quant = inputTensor->quantization;

    if (kTfLiteAffineQuantization == quant.type) {
        auto *quantParams = (TfLiteAffineQuantization *)quant.params;
        const float quantScale = quantParams->scale->data[0];
        const int quantOffset = quantParams->zero_point->data[0];

        switch (inputTensor->type) {
            case kTfLiteInt8: {
                mfccFeatureCalc =
                    FeatureCalc<int8_t>(inputTensor, cacheSize, [=, &mfcc](std::vector<int16_t> &audioDataWindow) {
                        return mfcc.MfccComputeQuant<int8_t>(audioDataWindow, quantScale, quantOffset);
                    });
                break;
            }
            case kTfLiteUInt8: {
                mfccFeatureCalc =
                    FeatureCalc<uint8_t>(inputTensor, cacheSize, [=, &mfcc](std::vector<int16_t> &audioDataWindow) {
                        return mfcc.MfccComputeQuant<uint8_t>(audioDataWindow, quantScale, quantOffset);
                    });
                break;
            }
            case kTfLiteInt16: {
                mfccFeatureCalc =
                    FeatureCalc<int16_t>(inputTensor, cacheSize, [=, &mfcc](std::vector<int16_t> &audioDataWindow) {
                        return mfcc.MfccComputeQuant<int16_t>(audioDataWindow, quantScale, quantOffset);
                    });
                break;
            }
            default:
                printf_err("Tensor type %s not supported\n", TfLiteTypeGetName(inputTensor->type));
        }

    } else {
        mfccFeatureCalc = FeatureCalc<float>(inputTensor, cacheSize, [&mfcc](std::vector<int16_t> &audioDataWindow) {
            return mfcc.MfccCompute(audioDataWindow);
        });
    }
    return mfccFeatureCalc;
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
    const void *ethosu_base_address = (void *)(SEC_ETHOS_U55_BASE);

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
    struct ethosu_version version;
    if (0 != (err = ethosu_get_version(&ethosu_drv, &version))) {
        printf_err("failed to fetch Ethos-U55 version info\n");
        return err;
    }

    info("Ethos-U55 version info:\n");
    info("\tArch:       v%u.%u.%u\n", version.id.arch_major_rev, version.id.arch_minor_rev, version.id.arch_patch_rev);
    info("\tDriver:     v%u.%u.%u\n",
         version.id.driver_major_rev,
         version.id.driver_minor_rev,
         version.id.driver_patch_rev);
    info("\tMACs/cc:    %u\n", (1 << version.cfg.macs_per_cc));
    info("\tCmd stream: v%u\n", version.cfg.cmd_stream_version);
    info("\tSHRAM size: %u\n", version.cfg.shram_size);

    return 0;
}

extern "C" {

ml_processing_state_t get_ml_processing_state()
{
    if (!ml_lock()) {
        return ML_UNKNOWN;
    }

    ml_processing_state_t result = ml_processing_state;

    ml_unlock();

    return result;
}

void on_ml_processing_change(ml_processing_change_handler_t handler, void *self)
{
    ml_processing_change_handler = handler;
    ml_processing_change_ptr = self;
}

int ml_interface_init()
{
    static arm::app::DsCnnModel model; /* Model wrapper object. */

    // Initialize the ethos U55
    if (arm_npu_init() != 0) {
        printf_err("Failed to arm npu\n");
        return -1;
    }

    /* Load the model. */
    if (!model.Init()) {
        printf_err("Failed to initialise model\n");
        return -1;
    }

    /* Instantiate application context. */
    caseContext.Set<arm::app::Model &>("model", model);
    caseContext.Set<int>("frameLength", g_FrameLength);
    caseContext.Set<int>("frameStride", g_FrameStride);
    caseContext.Set<float>("scoreThreshold", g_ScoreThreshold); /* Normalised score threshold. */

    static arm::app::Classifier classifier; /* classifier wrapper object. */
    caseContext.Set<arm::app::Classifier &>("classifier", classifier);

    static std::vector<std::string> labels;
    GetLabelsVector(labels);

    caseContext.Set<const std::vector<std::string> &>("labels", labels);

    PrintTensorFlowVersion();
    printf("*** ML interface initialised\r\n");
    return 0;
}

void ml_task(void *arg)
{
    (void)arg;

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

    ProcessAudio(caseContext);
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
            mqtt_send_inference_result(msg.state);
        } else {
            printf_err("osMessageQueueGet ml mqtt msg queue failed\r\n");
            return;
        }
    }
}

} // extern "C"
