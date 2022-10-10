/* Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "dsp_task.h"
#include "dsp_interfaces.h"
#include "model_config.h"
#include "audio_config.h"

#include "cmsis_os2.h"
#include "scheduler.h"

extern "C" {
#include "fvp_sai.h"
#include "hal/sai_api.h"
}

#include "audio_config.h"
#include "print_log.h"

// audio constants
__attribute__((section(".bss.NoInit.audio_buf"))) __attribute__((aligned(4)))
int16_t shared_audio_buffer[AUDIO_BUFFER_SIZE / 2];

// This queue is used to wait for the network to start
// otherwise DSP is starting to read audio from beginning
// and ML task is starting too late  (after network) and the audio 
// segment to classify has already been processed
static osMessageQueueId_t dsp_msg_queue = NULL;

// Audio driver data
void (*event_fn)(void *);
void *event_ptr = nullptr;

// Audio driver configuration & event management
static void AudioEvent(mdh_sai_t *self, void *ctx, mdh_sai_transfer_complete_t code)
{
    (void)self;
    (void)ctx;

    if (code == MDH_SAI_TRANSFER_COMPLETE_CANCELLED) {
        ERR_LOG("Transfer cancelled\n");
    }

    if (code == MDH_SAI_TRANSFER_COMPLETE_DONE) {
        if (event_fn) {
            event_fn(event_ptr);
        }
    }
}

static void sai_handler_error(mdh_sai_t *self, mdh_sai_event_t code)
{
    (void)self;
    (void)code;
    ERR_LOG("Error during SAI transfer\n");
}

static int AudioDrv_Setup(void (*event_handler)(void *), void *event_handler_ptr)
{
    fvp_sai_t *fvpsai = fvp_sai_init(CHANNELS, SAMPLE_BITS, static_cast<uint32_t>(SAMPLE_RATE), AUDIO_BLOCK_SIZE);

    if (!fvpsai) {
        ERR_LOG("Failed to set up FVP SAI!\n");
        return -1;
    }

    mdh_sai_t *sai = &fvpsai->sai;
    mdh_sai_status_t ret = mdh_sai_set_transfer_complete_callback(sai, AudioEvent);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        ERR_LOG("Failed to set transfer complete callback");
        return ret;
    }

    ret = mdh_sai_set_event_callback(sai, sai_handler_error);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        ERR_LOG("Failed to enable transfer error callback");
        return ret;
    }

    ret = mdh_sai_transfer(sai, (uint8_t *)shared_audio_buffer, AUDIO_BLOCK_NUM, NULL);

    if (ret != MDH_SAI_STATUS_NO_ERROR) {
        ERR_LOG("Failed to start audio transfer");
        return ret;
    }

    event_fn = event_handler;
    event_ptr = event_handler_ptr;

    return 0;
}

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

    int16_t *audioBuf = shared_audio_buffer;
    auto audioSource = DspAudioSource(audioBuf, 
        AUDIO_BLOCK_NUM);

    dsp_msg_queue = osMessageQueueNew(10, sizeof(dsp_msg_t), NULL);

    DSPML *dspMLConnection = (DSPML*)pvParameters;

    bool first_launch = true;

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

        if (first_launch) { 
            AudioDrv_Setup(&DspAudioSource::new_audio_block_received, &audioSource);
            first_launch = false;
        }

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