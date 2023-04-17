/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ML_INTERFACE_H
#define ML_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ml_task_inference_start();
void ml_task_inference_stop();

/* Initialises the interface to audio processing.
 */
int ml_interface_init(void);

void ml_process_audio(const int16_t *buffer, size_t size);

/* this task does actual ml processing and is gated by the net task which let's it run if no ota job is present */
void ml_task(void *arg);

/* task used to communicate ml results via mqtt */
void ml_mqtt_task(void *arg);

/* Parameters of the ml input
 */
int ml_frame_length();
int ml_frame_stride();

/* To be implemented by application to send inference result */
void mqtt_send_inference_result(const char *message);

#ifdef __cplusplus
}
#endif

#endif /* ! ML_INTERFACE_H */
