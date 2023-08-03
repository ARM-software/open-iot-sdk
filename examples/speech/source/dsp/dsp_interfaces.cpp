/* Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp_interfaces.h"
#include "audio_config.h"
#include "model_config.h"
#include "print_log.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
// For memcpy
#include <cstring>

extern "C" {
#include "mbed_critical/mbed_critical.h"
}

static float audio_timestamp = 0.0;

void set_audio_timestamp(float timestamp) {
    core_util_critical_section_enter();
    audio_timestamp = timestamp;
    core_util_critical_section_exit();
}

float get_audio_timestamp() {
    core_util_critical_section_enter();
    float timestamp = audio_timestamp;
    core_util_critical_section_exit();
    return timestamp;
}

DspAudioSource::DspAudioSource(const int16_t* audiobuffer, size_t block_count ):
        block_count{block_count},
        audiobuffer{audiobuffer} 
{

}

const int16_t *DspAudioSource::getCurrentBuffer()
{
#ifndef AUDIO_VSI
    // Update block ID
    current_block = (current_block + 1) % block_count;
#endif

    return(audiobuffer+this->current_block*(AUDIO_BLOCK_SIZE/2));
}

#ifdef AUDIO_VSI

void DspAudioSource::waitForNewBuffer()
{
    osSemaphoreAcquire(this->semaphore, osWaitForever);
}

void DspAudioSource::new_audio_block_received(void* ptr)
{
    auto* self = reinterpret_cast<DspAudioSource*>(ptr);
    
    // Update block ID
    self->current_block = self->block_under_write;
    self->block_under_write = ((self->block_under_write + 1) % self->block_count);

    // Wakeup task waiting
    osSemaphoreRelease(self->semaphore);
};

#endif

static bool dspml_lock(osMutexId_t ml_fifo_mutex)
{
    bool success = false;
    if ( ml_fifo_mutex ) {
        if (osMutexAcquire(ml_fifo_mutex, osWaitForever) == osOK ) {
            success = true;
        }
        else {
            ERR_LOG( "Failed to acquire ml_fifo_mutex" );
        }
    }
    return success;
}

static bool dspml_unlock(osMutexId_t ml_fifo_mutex)
{
    bool success = false;
    if ( ml_fifo_mutex ) {
        if (osMutexRelease( ml_fifo_mutex ) == osOK ) {
            success = true;
        }
        else {
            ERR_LOG( "Failed to release ml_fifo_mutex" );
        }
    }
    return success;
}


DSPML::DSPML(size_t bufferLengthInSamples ):nbSamples(bufferLengthInSamples)
{
    bufferA=(int16_t*)malloc(bufferLengthInSamples*sizeof(int16_t));
    bufferB=(int16_t*)malloc(bufferLengthInSamples*sizeof(int16_t));

    dspBuffer = bufferA;
    mlBuffer = bufferB;
}

DSPML::~DSPML()
{
    free(bufferA);
    free(bufferB);
}

void DSPML::copyToDSPBufferFrom(int16_t * buf)
{
    dspml_lock(this->mutex);
    memcpy(dspBuffer,buf,sizeof(int16_t)*nbSamples);
    dspml_unlock(this->mutex);

}

void DSPML::copyFromMLBufferInto(int16_t * buf)
{
    dspml_lock(this->mutex);
    memcpy(buf,mlBuffer,sizeof(int16_t)*nbSamples);
    dspml_unlock(this->mutex);
}

void DSPML::swapBuffersAndWakeUpMLThread()
{
    int16_t* tmp;

    dspml_lock(this->mutex);
    tmp=dspBuffer;
    dspBuffer=mlBuffer;
    mlBuffer=tmp;
    dspml_unlock(this->mutex);

    osSemaphoreRelease(this->semaphore);
}

void DSPML::waitForDSPData()
{
    osSemaphoreAcquire(this->semaphore, osWaitForever);
}

