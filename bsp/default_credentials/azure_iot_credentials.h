/* Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AZURE_IOT_CREDENTIALS_H
#define AZURE_IOT_CREDENTIALS_H

#define DEFAULT_CONNECTION_DETAILS ""

/* These values can be picked from device connection string which is of format :
   HostName=<host1>;DeviceId=<device1>;SharedAccessKey=<key1> HOST_NAME can be set to <host1>, DEVICE_ID can be set to
   <device1>, DEVICE_SYMMETRIC_KEY can be set to <key1>.  */
#ifndef HOST_NAME
#define HOST_NAME DEFAULT_CONNECTION_DETAILS
#endif

#ifndef DEVICE_ID
#define DEVICE_ID DEFAULT_CONNECTION_DETAILS
#endif

#ifndef DEVICE_SYMMETRIC_KEY
#define DEVICE_SYMMETRIC_KEY DEFAULT_CONNECTION_DETAILS
#endif

#ifndef MODULE_ID
#define MODULE_ID DEFAULT_CONNECTION_DETAILS
#endif

#endif // AZURE_IOT_CREDENTIALS_H
