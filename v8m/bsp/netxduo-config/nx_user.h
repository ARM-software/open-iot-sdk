/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NX_USER_H
#define NX_USER_H

// Enable Azure IoT Middleware for Azure RTOS
#define NX_ENABLE_EXTENDED_NOTIFY_SUPPORT
#define NX_SECURE_ENABLE
#define NXD_MQTT_CLOUD_ENABLE

// Enable Azure Defender for IoT security module
#define NX_ENABLE_IP_PACKET_FILTER

#endif // NX_USER_H
