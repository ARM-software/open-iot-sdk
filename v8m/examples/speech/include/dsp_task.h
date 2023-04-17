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
#ifndef DSP_TASK_H
#define DSP_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { DSP_EVENT_START, DSP_EVENT_STOP } dsp_event_t;
typedef struct {
    dsp_event_t event;
} dsp_msg_t;

void dsp_task_start();
void dsp_task_stop();

void dsp_task(void *pvParameters);
void *getDspMLConnection();

#ifdef __cplusplus
}
#endif

#endif /* ! DSP_TASK_H*/
