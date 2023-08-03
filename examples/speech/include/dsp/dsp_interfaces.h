/* Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DSP_INTERFACE_H_
#define _DSP_INTERFACE_H_

#include "cmsis_os2.h"

extern void set_audio_timestamp(float timestamp);
extern float get_audio_timestamp();

// Communication between DspAudioSource and ISR
struct DspAudioSource { 
    public:
    DspAudioSource(const int16_t* audiobuffer, size_t block_count );

    const int16_t *getCurrentBuffer();

#ifdef AUDIO_VSI
    void waitForNewBuffer();

    static void new_audio_block_received(void* ptr);
#endif

private:
    size_t block_count;
#ifdef AUDIO_VSI
    size_t block_under_write = 0;
#endif
    size_t current_block = 0;
    const int16_t* audiobuffer;
    osSemaphoreId_t semaphore = osSemaphoreNew(1, 0, NULL);
};

class DSPML {
public:
    DSPML(size_t bufferLengthInSamples );
    ~DSPML();

    void copyToDSPBufferFrom(int16_t * buf);
    void copyFromMLBufferInto(int16_t * buf);

    void swapBuffersAndWakeUpMLThread();
    void waitForDSPData();
    size_t getNbSamples() {return nbSamples;};

private:
    osMutexId_t mutex = osMutexNew(NULL);
    osSemaphoreId_t semaphore = osSemaphoreNew(1, 0, NULL);
    int16_t *bufferA,*bufferB,*dspBuffer,*mlBuffer;
    size_t nbSamples;
};

#endif
