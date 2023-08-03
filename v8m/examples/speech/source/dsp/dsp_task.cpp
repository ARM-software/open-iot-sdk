/* Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "dsp_task.h"
#include "dsp_interfaces.h"
#include "model_config.h"
#include "audio_config.h"

#include "cmsis_os2.h"
#include "scheduler.h"
#include "print_log.h"

#ifdef AUDIO_VSI

#include "Driver_SAI.h"

// audio constants
__attribute__((section(".bss.NoInit.audio_buf"))) __attribute__((aligned(4)))
int16_t shared_audio_buffer[AUDIO_BUFFER_SIZE / 2];

/* Audio events definitions */
#define SEND_COMPLETED_Pos         0U                              /* Event: Send complete bit position */
#define RECEIVE_COMPLETED_Pos      1U                              /* Event: Receive complete bit position */
#define TX_UNDERFLOW_Pos           2U                              /* Event: Tx underflow bit position */
#define RX_OVERFLOW_Pos            3U                              /* Event: Rx overflow bit position */
#define FRAME_ERROR_Pos            4U                              /* Event: Frame error bit position */
#define SEND_COMPLETE_Msk          (1UL << SEND_COMPLETED_Pos)     /* Event: Send complete Mask */
#define RECEIVE_COMPLETE_Msk       (1UL << RECEIVE_COMPLETED_Pos)  /* Event: Receive complete Mask */
#define TX_UNDERFLOW_Msk           (1UL << TX_UNDERFLOW_Pos)       /* Event: Tx underflow Mask */
#define RX_OVERFLOW_Msk            (1UL << RX_OVERFLOW_Pos)        /* Event: Rx overflow Mask */
#define FRAME_ERROR_Msk            (1UL << FRAME_ERROR_Pos)        /* Event: Frame error Mask */

extern ARM_DRIVER_SAI Driver_SAI0;

// Audio driver data
void (*event_fn)(void *);
void *event_ptr = nullptr;

// Audio driver callback function for event management
static void ARM_SAI_SignalEvent(uint32_t event)
{
    if ((event & SEND_COMPLETE_Msk) == ARM_SAI_EVENT_SEND_COMPLETE)
    {
        if (event_fn)
        {
            event_fn(event_ptr);
        }
    }
    if ((event & RECEIVE_COMPLETE_Msk) == ARM_SAI_EVENT_RECEIVE_COMPLETE)
    {
        if (event_fn)
        {
            event_fn(event_ptr);
        }
    }
    if ((event & TX_UNDERFLOW_Msk) == ARM_SAI_EVENT_TX_UNDERFLOW)
    {
        ERR_LOG("Error TX is enabled but no data is being sent\n");
    }
    if ((event & RX_OVERFLOW_Msk) == ARM_SAI_EVENT_RX_OVERFLOW)
    {
        ERR_LOG("Error RX is enabled but no data is being received\n");
    }
    if ((event & FRAME_ERROR_Msk) == ARM_SAI_EVENT_FRAME_ERROR)
    {
        ERR_LOG("Frame error occured\n");
    }
}


static int AudioDrv_Setup(void (*event_handler)(void *), void *event_handler_ptr)
{
    if (Driver_SAI0.Initialize(ARM_SAI_SignalEvent) != ARM_DRIVER_OK) {
        ERR_LOG("Failed to set up FVP VSI!\n");
        return -1;
    }

    if (Driver_SAI0.PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK) {
        ERR_LOG("Failed to set the driver to operate with full power!\n");
        return -1;
    }

    if (Driver_SAI0.Control(ARM_SAI_CONTROL_RX, 1, 0) != ARM_DRIVER_OK) {
        ERR_LOG("Failed to enable the VSI receiver!\n");
        return -1;
    }

    if (Driver_SAI0.Control(ARM_SAI_CONFIGURE_RX | ARM_SAI_PROTOCOL_USER | ARM_SAI_DATA_SIZE(16),
                                      AUDIO_BLOCK_SIZE,
                                      static_cast<uint32_t>(SAMPLE_RATE)) != ARM_DRIVER_OK) {
        ERR_LOG("Failed to configure the receiver!\n");
        return -1;
    }

    if (Driver_SAI0.Receive(reinterpret_cast<uint32_t *>(shared_audio_buffer), AUDIO_BLOCK_NUM) != ARM_DRIVER_OK) {
        ERR_LOG("Failed to start receiving the data!\n");
        return -1;
    }

    event_fn = event_handler;
    event_ptr = event_handler_ptr;

    return 0;
}

#else /* !defined(AUDIO_VSI) */

#include "InputFiles.hpp"

#endif // AUDIO_VSI

// This queue is used to wait for the network to start
// otherwise DSP is starting to read audio from beginning
// and ML task is starting too late  (after network) and the audio
// segment to classify has already been processed
static osMessageQueueId_t dsp_msg_queue = NULL;

extern "C" {

void dsp_task_start()
{
    const dsp_msg_t msg = {DSP_EVENT_START};
    printf("dsp task start\r\n");
    osMessageQueuePut(dsp_msg_queue, &msg, /* priority */ 0, /*timeout */ 0);
}

void dsp_task_stop()
{
    const dsp_msg_t msg = {DSP_EVENT_STOP};
    printf("dsp task stop\r\n");
    osMessageQueuePut(dsp_msg_queue, &msg, /* priority */ 0, /*timeout */ 0);
}
} // extern "C"

void *getDspMLConnection()
{
   auto dspMLConnection = new DSPML(AUDIOFEATURELENGTH);
   return((void*)dspMLConnection);
}

void dsp_task(void *pvParameters)
{
    printf("DSP start\r\n");

#ifdef AUDIO_VSI
    bool first_launch = true;
    const int16_t *audioBuf = shared_audio_buffer;
    auto audioSource = DspAudioSource(audioBuf, AUDIO_BLOCK_NUM);
#else
    const int16_t *audioBuf = GetAudioArray(0);
    // This integer division for calculating the number of blocks means that,
    // any remainder data at the end of the audio clip that's smaller than a
    // block will not be accounted for. This will not have a major impact on
    // the inference result as a block is only a small fraction of a second.
    const size_t audioBlockNum = (size_t)GetAudioArraySize(0) / (AUDIO_BLOCK_SIZE / sizeof(uint16_t));
    auto audioSource = DspAudioSource(audioBuf, audioBlockNum);
#endif

    dsp_msg_queue = osMessageQueueNew(10, sizeof(dsp_msg_t), NULL);

    DSPML *dspMLConnection = (DSPML*)pvParameters;

    while (1) { 
        // Wait for the start message
        while (1) {
            dsp_msg_t msg;
            if (osMessageQueueGet(dsp_msg_queue, &msg, /* priority */ 0, osWaitForever) == osOK) {
                if (msg.event == DSP_EVENT_START) {
                    break;
                } /* else it's DSP_EVENT_STOP so we keep waiting the loop */
            }
        }

#ifdef AUDIO_VSI
        if (first_launch) { 
            AudioDrv_Setup(&DspAudioSource::new_audio_block_received, &audioSource);
            first_launch = false;
        }
#endif

        // Launch the CMSIS-DSP synchronous data flow.
        // This compute graph is defined in graph.py
        // It can be regenerated with
        // pip install cmsisdsp
        // python graph.py
        int error;
        uint32_t nbSched=scheduler(&error,&audioSource, dspMLConnection,dsp_msg_queue);
        printf("Synchronous Dataflow Scheduler ended with error %d after %i schedule loops\r\n",error,nbSched);
    }
}
