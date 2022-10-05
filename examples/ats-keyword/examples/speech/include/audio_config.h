/* Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AUDIO_CONFIG_H_
#define _AUDIO_CONFIG_H_

#define AUDIO_BLOCK_NUM   (4)
#define AUDIO_BLOCK_SIZE  (3200)
#define AUDIO_BUFFER_SIZE (AUDIO_BLOCK_NUM * AUDIO_BLOCK_SIZE)

#define DSP_BLOCK_SIZE        320
#define SAMPLE_RATE           16000
#define CHANNELS              1U
#define SAMPLE_BITS           16U
#define NOISE_LEVEL_REDUCTION 30

#endif
