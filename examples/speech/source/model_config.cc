/*
 * Copyright (c) 2022 Arm Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "model_config.h"

#include <stdint.h>

const uint32_t g_NumMfcc = 10;
const uint32_t g_NumAudioWins = 49;

const int g_FrameLength = 512;
const int g_FrameStride = 160;
const int g_ctxLen = 98;
const float g_ScoreThreshold = 0.5;
