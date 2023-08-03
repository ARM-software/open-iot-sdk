
/* Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ml_interface.h"
#include "AudioUtils.hpp"
#include "BufAttributes.hpp"
#include "KwsClassifier.hpp"
#include "KwsProcessing.hpp"
#include "KwsResult.hpp"
#include "Labels.hpp"
#include "MicroNetKwsMfcc.hpp"
#include "MicroNetKwsModel.hpp"
#include "TensorFlowLiteMicro.hpp"
#include "UseCaseCommonUtils.hpp"
#include CMSIS_device_header
#include "cmsis_os2.h"
#include "device_mps3.h"   /* FPGA level definitions and functions. */
#include "ethos-u55.h"     /* Mem map and configuration definitions of the Ethos U55 */
#include "ethosu_driver.h" /* Arm Ethos-U55 driver header */
#include "hal.h"
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

#ifdef AUDIO_VSI

#include "Driver_SAI.h"
extern "C" {
#include "mbed_critical/mbed_critical.h"
}

#define AUDIO_BLOCK_NUM   (4)
#define AUDIO_BLOCK_SIZE  (3200)
#define AUDIO_BUFFER_SIZE (AUDIO_BLOCK_NUM * AUDIO_BLOCK_SIZE)

// audio constants
__attribute__((section(".bss.NoInit.audio_buf"))) __attribute__((aligned(4)))
int16_t shared_audio_buffer[AUDIO_BUFFER_SIZE / 2];
const int kAudioSampleFrequency = 16000;

/* Audio events definitions */
#define SEND_COMPLETED_Pos    0U                             /* Event: Send complete bit position */
#define RECEIVE_COMPLETED_Pos 1U                             /* Event: Receive complete bit position */
#define TX_UNDERFLOW_Pos      2U                             /* Event: Tx underflow bit position */
#define RX_OVERFLOW_Pos       3U                             /* Event: Rx overflow bit position */
#define FRAME_ERROR_Pos       4U                             /* Event: Frame error bit position */
#define SEND_COMPLETE_Msk     (1UL << SEND_COMPLETED_Pos)    /* Event: Send complete Mask */
#define RECEIVE_COMPLETE_Msk  (1UL << RECEIVE_COMPLETED_Pos) /* Event: Receive complete Mask */
#define TX_UNDERFLOW_Msk      (1UL << TX_UNDERFLOW_Pos)      /* Event: Tx underflow Mask */
#define RX_OVERFLOW_Msk       (1UL << RX_OVERFLOW_Pos)       /* Event: Rx overflow Mask */
#define FRAME_ERROR_Msk       (1UL << FRAME_ERROR_Pos)       /* Event: Frame error Mask */

extern ARM_DRIVER_SAI Driver_SAI0;

#else /* !defined(AUDIO_VSI) */

#include "InputFiles.hpp"

#endif /* AUDIO_VSI */

// Define tensor arena and declare functions required to access the model
namespace arm {
namespace app {
uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;
namespace kws {
extern uint8_t *GetModelPointer();
extern size_t GetModelLen();
} /* namespace kws */
} /* namespace app */
} /* namespace arm */

namespace {

#define ML_EVENT_START (1 << 0)
#define ML_EVENT_STOP  (1 << 1)

typedef struct {
    ml_processing_state_t state;
} ml_mqtt_msg_t;

// Import
using namespace arm::app;

// Processing state
static osEventFlagsId_t ml_process_flags = NULL;
static osMessageQueueId_t ml_mqtt_msg_queue = NULL;
osMutexId_t ml_mutex = NULL;
osMutexId_t serial_mutex = NULL;
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
    std::pair<const char *, ml_processing_state_t>{"go", ML_HEARD_GO},
    std::pair<const char *, ml_processing_state_t>{"stop", ML_HEARD_STOP},
};

extern "C" {
const char *get_inference_result_string(ml_processing_state_t ref_state)
{
    return (label_to_state[ref_state].first);
}

void ml_task_inference_start()
{
    info("Signal task inference start\r\n");
    osEventFlagsClear(ml_process_flags, ML_EVENT_STOP);
    osEventFlagsSet(ml_process_flags, ML_EVENT_START);
}

void ml_task_inference_stop()
{
    info("Signal task inference stop\r\n");
    osEventFlagsClear(ml_process_flags, ML_EVENT_START);
    osEventFlagsSet(ml_process_flags, ML_EVENT_STOP);
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

void set_ml_processing_state(ml_processing_state_t new_state)
{
    if (!ml_lock()) {
        return;
    }

    // In this use case, only changes in state are relevant. Additionally,
    // this avoids reporting the same keyword detected twice in adjacent,
    // overlapping inference windows.
    static ml_processing_state_t ml_processing_state{ML_SILENCE};
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

#ifdef AUDIO_VSI

// Audio driver data
void (*event_fn)(void *);
void *event_ptr = nullptr;

// Audio driver callback function for event management
static void ARM_SAI_SignalEvent(uint32_t event)
{
    if ((event & SEND_COMPLETE_Msk) == ARM_SAI_EVENT_SEND_COMPLETE) {
        if (event_fn) {
            event_fn(event_ptr);
        }
    }
    if ((event & RECEIVE_COMPLETE_Msk) == ARM_SAI_EVENT_RECEIVE_COMPLETE) {
        if (event_fn) {
            event_fn(event_ptr);
        }
    }
    if ((event & TX_UNDERFLOW_Msk) == ARM_SAI_EVENT_TX_UNDERFLOW) {
        printf_err("Error TX is enabled but no data is being sent\n");
    }
    if ((event & RX_OVERFLOW_Msk) == ARM_SAI_EVENT_RX_OVERFLOW) {
        printf_err("Error RX is enabled but no data is being received\n");
    }
    if ((event & FRAME_ERROR_Msk) == ARM_SAI_EVENT_FRAME_ERROR) {
        printf_err("Frame error occured\n");
    }
}

int AudioDrv_Setup(void (*event_handler)(void *), void *event_handler_ptr)
{
    if (Driver_SAI0.Initialize(ARM_SAI_SignalEvent) != ARM_DRIVER_OK) {
        printf_err("Failed to set up FVP VSI!\n");
        return -1;
    }

    if (Driver_SAI0.PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK) {
        printf_err("Failed to set the driver to operate with full power!\n");
        return -1;
    }

    if (Driver_SAI0.Control(ARM_SAI_CONTROL_RX, 1, 0) != ARM_DRIVER_OK) {
        printf_err("Failed to enable the VSI receiver!\n");
        return -1;
    }

    if (Driver_SAI0.Control(ARM_SAI_CONFIGURE_RX | ARM_SAI_PROTOCOL_USER | ARM_SAI_DATA_SIZE(16),
                            AUDIO_BLOCK_SIZE,
                            static_cast<uint32_t>(kAudioSampleFrequency))
        != ARM_DRIVER_OK) {
        printf_err("Failed to configure the receiver!\n");
        return -1;
    }

    if (Driver_SAI0.Receive(reinterpret_cast<uint32_t *>(shared_audio_buffer), AUDIO_BLOCK_NUM) != ARM_DRIVER_OK) {
        printf_err("Failed to start receiving the data!\n");
        return -1;
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
        core_util_critical_section_enter();
        auto result = block_under_write;
        core_util_critical_section_exit();
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

#endif /* AUDIO_VSI */

/**
 * @brief           Presents inference results using the data presentation
 *                  object.
 * @param[in]       platform    Reference to the hal platform object.
 * @param[in]       results     Vector of classification results to be displayed.
 * @return          true if successful, false otherwise.
 **/
static bool PresentInferenceResult(const arm::app::kws::KwsResult &result);

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
GetFeatureCalculator(audio::MicroNetKwsMFCC &mfcc, TfLiteTensor *inputTensor, size_t cacheSize);

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
        static_cast<int>((arm::app::MicroNetKwsModel::ms_inputRowsIdx > arm::app::MicroNetKwsModel::ms_inputColsIdx)
                             ? arm::app::MicroNetKwsModel::ms_inputRowsIdx
                             : arm::app::MicroNetKwsModel::ms_inputColsIdx);

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
    const uint32_t kNumCols = inputShape->data[arm::app::MicroNetKwsModel::ms_inputColsIdx];
    const uint32_t kNumRows = inputShape->data[arm::app::MicroNetKwsModel::ms_inputRowsIdx];

    audio::MicroNetKwsMFCC mfcc = audio::MicroNetKwsMFCC(kNumCols, frameLength);
    mfcc.Init();

    /* Deduce the data length required for 1 inference from the network parameters. */
    auto audioDataWindowSize = kNumRows * frameStride + (frameLength - frameStride); // 16000
#ifdef AUDIO_VSI
    auto mfccWindowSize = frameLength;   // 640
#endif                                   /* AUDIO_VSI */
    auto mfccWindowStride = frameStride; // 320

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
    const float secondsPerSample = 1.0 / audio::MicroNetKwsMFCC::ms_defaultSamplingFreq;

    /* Calculate number of the feature vectors in the window overlap region.
     * These feature vectors will be reused.*/
    auto numberOfReusedFeatureVectors = nMfccVectorsInAudioStride;

    /* Construct feature calculation function. */
    auto mfccFeatureCalc = GetFeatureCalculator(mfcc, inputTensor, numberOfReusedFeatureVectors);

    if (!mfccFeatureCalc) {
        printf_err("No feature calculator available");
        return;
    }

#ifdef AUDIO_VSI

    // Initialize the sliding window
    auto circularSlider = CircularSlidingWindow<int16_t>(
        shared_audio_buffer, AUDIO_BLOCK_SIZE / sizeof(int16_t), AUDIO_BLOCK_NUM, mfccWindowSize, mfccWindowStride);

    // Initialize the audio driver. It is delayed until that point to avoid drop
    // of starting frames.
    AudioDrv_Setup(&decltype(circularSlider)::signal_block_written, &circularSlider);

    bool first_iteration = true;
    auto mfccAudioData = std::vector<int16_t>(mfccWindowSize, 0);
    size_t audio_index = 0;

#endif /* AUDIO_VSI */

    while (true) {

#ifdef AUDIO_VSI

        printf("Running inference as audio input is received from the Virtual Streaming Interface\r\n");

        while (true) {
            uint32_t flags = osEventFlagsWait(ml_process_flags, ML_EVENT_STOP, osFlagsWaitAny, 0);
            if (flags == ML_EVENT_STOP) {
                /* jump out to outer loop */
                info("Stopping audio processing\r\n");
                break;
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
                outputTensor, classificationResult, ctx.Get<std::vector<std::string> &>("labels"), 1, true);

            auto result = kws::KwsResult(
                classificationResult, audio_index * secondsPerSample * audioDataStride, audio_index, scoreThreshold);

            if (result.m_resultVec.empty()) {
                set_ml_processing_state(ML_UNKNOWN);
            } else {
                set_ml_processing_state(convert_inference_result(result.m_resultVec[0].m_label));
            }

            if (PresentInferenceResult(result) != true) {
                printf_err("Failed to present inference result");
                return;
            }
            first_iteration = false;
            ++audio_index;
        } /* while (true) */

#else /* !defined(AUDIO_VSI) */

        printf("Running inference on an audio clip in local memory\r\n");

        const uint32_t numMfccFeatures = inputShape->data[MicroNetKwsModel::ms_inputColsIdx];
        const uint32_t numMfccFrames = inputShape->data[arm::app::MicroNetKwsModel::ms_inputRowsIdx];

        KwsPreProcess preProcess = KwsPreProcess(
            inputTensor, numMfccFeatures, numMfccFrames, ctx.Get<int>("frameLength"), ctx.Get<int>("frameStride"));

        std::vector<ClassificationResult> singleInfResult;
        KwsPostProcess postProcess = KwsPostProcess(outputTensor,
                                                    ctx.Get<arm::app::KwsClassifier &>("classifier"),
                                                    ctx.Get<std::vector<std::string> &>("labels"),
                                                    singleInfResult);

        /* Creating a sliding window through the whole audio clip. */
        auto audioDataSlider = audio::SlidingWindow<const int16_t>(
            GetAudioArray(0), GetAudioArraySize(0), preProcess.m_audioDataWindowSize, preProcess.m_audioDataStride);

        /* Start sliding through audio clip. */
        while (audioDataSlider.HasNext()) {
            uint32_t flags = osEventFlagsWait(ml_process_flags, ML_EVENT_STOP, osFlagsWaitAny, 0);
            if (flags == ML_EVENT_STOP) {
                /* Jump out to the outer loop, which may restart inference on an ML_EVENT_START signal */
                info("Inference stopped by a signal.\r\n");
                break;
            }

            const int16_t *inferenceWindow = audioDataSlider.Next();
            if (!preProcess.DoPreProcess(inferenceWindow, audioDataSlider.Index())) {
                printf_err("Pre-processing failed.");
                return;
            }

            if (!model.RunInference()) {
                printf_err("Inference failed.");
                return;
            }

            if (!postProcess.DoPostProcess()) {
                printf_err("Post-processing failed.");
                return;
            }

            auto result = kws::KwsResult(singleInfResult,
                                         audioDataSlider.Index() * secondsPerSample * preProcess.m_audioDataStride,
                                         audioDataSlider.Index(),
                                         scoreThreshold);

            if (result.m_resultVec.empty()) {
                set_ml_processing_state(ML_UNKNOWN);
            } else {
                set_ml_processing_state(convert_inference_result(result.m_resultVec[0].m_label));
            }

            if (PresentInferenceResult(result) != true) {
                printf_err("Failed to present inference result");
                return;
            }
        } /* while (audioDataSlider.HasNext()) */

#endif /* AUDIO_VSI */

        while (true) {
            uint32_t flags = osEventFlagsWait(ml_process_flags, ML_EVENT_START, osFlagsWaitAny, osWaitForever);
            if (flags == ML_EVENT_START) {
                printf_err("Restarting audio processing %u\r\n", flags);
                break;
            }
        }
    } /* while (true) */
}

static bool PresentInferenceResult(const arm::app::kws::KwsResult &result)
{
    if (!serial_lock()) {
        return false;
    }

    /* Display each result */
    if (result.m_resultVec.empty()) {
        info("For timestamp: %f (inference #: %" PRIu32 "); label: %s; threshold: %f\n",
             (double)result.m_timeStamp,
             result.m_inferenceNumber,
             "<none>",
             0.);
    } else {
        for (uint32_t i = 0; i < result.m_resultVec.size(); ++i) {
            info("For timestamp: %f (inference #: %" PRIu32 "); label: %s, score: %f; threshold: %f\n",
                 (double)result.m_timeStamp,
                 result.m_inferenceNumber,
                 result.m_resultVec[i].m_label.c_str(),
                 result.m_resultVec[i].m_normalisedVal,
                 (double)result.m_threshold);
        }
    }

    serial_unlock();

    return true;
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
GetFeatureCalculator(audio::MicroNetKwsMFCC &mfcc, TfLiteTensor *inputTensor, size_t cacheSize)
{
    std::function<void(std::vector<int16_t> &, size_t, bool, size_t)> mfccFeatureCalc;
    TfLiteQuantization quant = inputTensor->quantization;

    if (kTfLiteAffineQuantization == quant.type) {
        auto *quantParams = static_cast<TfLiteAffineQuantization *>(quant.params);
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

#ifdef USE_ETHOS
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
void ETHOS_U55_Handler(void)
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
    NVIC_SetVector(ethosu_irqnum, (uint32_t)ETHOS_U55_Handler);

    /* Enable the IRQ */
    NVIC_EnableIRQ(ethosu_irqnum);

    debug("EthosU IRQ#: %u, Handler: 0x%p\n", ethosu_irqnum, ETHOS_U55_Handler);
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
    void *const ethosu_base_address = reinterpret_cast<void *const>(SEC_ETHOS_U55_BASE);

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
#endif /* USE_ETHOS */

extern "C" {

bool serial_lock()
{
    bool success = false;
    if (serial_mutex) {
        osStatus_t status = osMutexAcquire(serial_mutex, osWaitForever);
        if (status != osOK) {
            printf_err("osMutexAcquire serial_mutex failed %d\r\n", status);
        } else {
            success = true;
        }
    }
    return success;
}

bool serial_unlock()
{
    bool success = false;
    if (serial_mutex) {
        osStatus_t status = osMutexRelease(serial_mutex);
        if (status != osOK) {
            printf_err("osMutexRelease serial_mutex failed %d\r\n", status);
        } else {
            success = true;
        }
    }
    return success;
}

void on_ml_processing_change(ml_processing_change_handler_t handler, void *self)
{
    ml_processing_change_handler = handler;
    ml_processing_change_ptr = self;
}

int ml_interface_init()
{
    static arm::app::MicroNetKwsModel model; /* Model wrapper object. */

#ifdef USE_ETHOS
    // Initialize the ethos U55
    if (arm_npu_init() != 0) {
        printf_err("Failed to arm npu\n");
        return -1;
    }
#endif /* USE_ETHOS */

    /* Load the model. */
    if (!model.Init(::arm::app::tensorArena,
                    sizeof(::arm::app::tensorArena),
                    ::arm::app::kws::GetModelPointer(),
                    ::arm::app::kws::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return -1;
    }

    /* Instantiate application context. */
    caseContext.Set<arm::app::Model &>("model", model);
    caseContext.Set<int>("frameLength", arm::app::kws::g_FrameLength);
    caseContext.Set<int>("frameStride", arm::app::kws::g_FrameStride);
    caseContext.Set<float>("scoreThreshold", arm::app::kws::g_ScoreThreshold); /* Normalised score threshold. */

    static KwsClassifier classifier; /* classifier wrapper object. */
    caseContext.Set<arm::app::KwsClassifier &>("classifier", classifier);

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

    serial_mutex = osMutexNew(NULL);
    if (!serial_mutex) {
        printf_err("Failed to create ml_mutex\r\n");
        return;
    }

    ml_process_flags = osEventFlagsNew(NULL);
    if (!ml_process_flags) {
        printf_err("Failed to create ml process flags\r\n");
        return;
    }

    while (true) {
        uint32_t flags = osEventFlagsWait(ml_process_flags, ML_EVENT_START, osFlagsWaitAny, osWaitForever);
        if (flags) {
            if (flags == ML_EVENT_START) {
                info("Initial start of audio processing\r\n");
                break;
            }
            if (flags & osFlagsError) {
                printf_err("Failed to wait for ml_process_flags: %08X\r\n", flags);
                return;
            }
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
            mqtt_send_inference_result(get_inference_result_string(msg.state));
        } else {
            printf_err("osMessageQueueGet ml mqtt msg queue failed\r\n");
            return;
        }
    }
}

} // extern "C"
