/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/* This file contains certs needed to communicate with Azure (IoT) */

#include "nx_azure_iot_cert.h"

/* DigiCert Baltimore Root --Used Globally--  */
const unsigned char _nx_azure_iot_root_cert[] = {
    0x30, 0x82, 0x03, 0x77, 0x30, 0x82, 0x02, 0x5f, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x04, 0x02, 0x00, 0x00, 0xb9,
    0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x5a, 0x31, 0x0b,
    0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 0x45, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04,
    0x0a, 0x13, 0x09, 0x42, 0x61, 0x6c, 0x74, 0x69, 0x6d, 0x6f, 0x72, 0x65, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
    0x04, 0x0b, 0x13, 0x0a, 0x43, 0x79, 0x62, 0x65, 0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x31, 0x22, 0x30, 0x20, 0x06,
    0x03, 0x55, 0x04, 0x03, 0x13, 0x19, 0x42, 0x61, 0x6c, 0x74, 0x69, 0x6d, 0x6f, 0x72, 0x65, 0x20, 0x43, 0x79, 0x62,
    0x65, 0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x30, 0x30,
    0x35, 0x31, 0x32, 0x31, 0x38, 0x34, 0x36, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x32, 0x35, 0x30, 0x35, 0x31, 0x32, 0x32,
    0x33, 0x35, 0x39, 0x30, 0x30, 0x5a, 0x30, 0x5a, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
    0x49, 0x45, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x09, 0x42, 0x61, 0x6c, 0x74, 0x69, 0x6d,
    0x6f, 0x72, 0x65, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x0a, 0x43, 0x79, 0x62, 0x65, 0x72,
    0x54, 0x72, 0x75, 0x73, 0x74, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x19, 0x42, 0x61, 0x6c,
    0x74, 0x69, 0x6d, 0x6f, 0x72, 0x65, 0x20, 0x43, 0x79, 0x62, 0x65, 0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x52,
    0x6f, 0x6f, 0x74, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
    0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xa3, 0x04,
    0xbb, 0x22, 0xab, 0x98, 0x3d, 0x57, 0xe8, 0x26, 0x72, 0x9a, 0xb5, 0x79, 0xd4, 0x29, 0xe2, 0xe1, 0xe8, 0x95, 0x80,
    0xb1, 0xb0, 0xe3, 0x5b, 0x8e, 0x2b, 0x29, 0x9a, 0x64, 0xdf, 0xa1, 0x5d, 0xed, 0xb0, 0x09, 0x05, 0x6d, 0xdb, 0x28,
    0x2e, 0xce, 0x62, 0xa2, 0x62, 0xfe, 0xb4, 0x88, 0xda, 0x12, 0xeb, 0x38, 0xeb, 0x21, 0x9d, 0xc0, 0x41, 0x2b, 0x01,
    0x52, 0x7b, 0x88, 0x77, 0xd3, 0x1c, 0x8f, 0xc7, 0xba, 0xb9, 0x88, 0xb5, 0x6a, 0x09, 0xe7, 0x73, 0xe8, 0x11, 0x40,
    0xa7, 0xd1, 0xcc, 0xca, 0x62, 0x8d, 0x2d, 0xe5, 0x8f, 0x0b, 0xa6, 0x50, 0xd2, 0xa8, 0x50, 0xc3, 0x28, 0xea, 0xf5,
    0xab, 0x25, 0x87, 0x8a, 0x9a, 0x96, 0x1c, 0xa9, 0x67, 0xb8, 0x3f, 0x0c, 0xd5, 0xf7, 0xf9, 0x52, 0x13, 0x2f, 0xc2,
    0x1b, 0xd5, 0x70, 0x70, 0xf0, 0x8f, 0xc0, 0x12, 0xca, 0x06, 0xcb, 0x9a, 0xe1, 0xd9, 0xca, 0x33, 0x7a, 0x77, 0xd6,
    0xf8, 0xec, 0xb9, 0xf1, 0x68, 0x44, 0x42, 0x48, 0x13, 0xd2, 0xc0, 0xc2, 0xa4, 0xae, 0x5e, 0x60, 0xfe, 0xb6, 0xa6,
    0x05, 0xfc, 0xb4, 0xdd, 0x07, 0x59, 0x02, 0xd4, 0x59, 0x18, 0x98, 0x63, 0xf5, 0xa5, 0x63, 0xe0, 0x90, 0x0c, 0x7d,
    0x5d, 0xb2, 0x06, 0x7a, 0xf3, 0x85, 0xea, 0xeb, 0xd4, 0x03, 0xae, 0x5e, 0x84, 0x3e, 0x5f, 0xff, 0x15, 0xed, 0x69,
    0xbc, 0xf9, 0x39, 0x36, 0x72, 0x75, 0xcf, 0x77, 0x52, 0x4d, 0xf3, 0xc9, 0x90, 0x2c, 0xb9, 0x3d, 0xe5, 0xc9, 0x23,
    0x53, 0x3f, 0x1f, 0x24, 0x98, 0x21, 0x5c, 0x07, 0x99, 0x29, 0xbd, 0xc6, 0x3a, 0xec, 0xe7, 0x6e, 0x86, 0x3a, 0x6b,
    0x97, 0x74, 0x63, 0x33, 0xbd, 0x68, 0x18, 0x31, 0xf0, 0x78, 0x8d, 0x76, 0xbf, 0xfc, 0x9e, 0x8e, 0x5d, 0x2a, 0x86,
    0xa7, 0x4d, 0x90, 0xdc, 0x27, 0x1a, 0x39, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x45, 0x30, 0x43, 0x30, 0x1d, 0x06,
    0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xe5, 0x9d, 0x59, 0x30, 0x82, 0x47, 0x58, 0xcc, 0xac, 0xfa, 0x08,
    0x54, 0x36, 0x86, 0x7b, 0x3a, 0xb5, 0x04, 0x4d, 0xf0, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
    0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x03, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01,
    0xff, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
    0x05, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x85, 0x0c, 0x5d, 0x8e, 0xe4, 0x6f, 0x51, 0x68, 0x42, 0x05, 0xa0,
    0xdd, 0xbb, 0x4f, 0x27, 0x25, 0x84, 0x03, 0xbd, 0xf7, 0x64, 0xfd, 0x2d, 0xd7, 0x30, 0xe3, 0xa4, 0x10, 0x17, 0xeb,
    0xda, 0x29, 0x29, 0xb6, 0x79, 0x3f, 0x76, 0xf6, 0x19, 0x13, 0x23, 0xb8, 0x10, 0x0a, 0xf9, 0x58, 0xa4, 0xd4, 0x61,
    0x70, 0xbd, 0x04, 0x61, 0x6a, 0x12, 0x8a, 0x17, 0xd5, 0x0a, 0xbd, 0xc5, 0xbc, 0x30, 0x7c, 0xd6, 0xe9, 0x0c, 0x25,
    0x8d, 0x86, 0x40, 0x4f, 0xec, 0xcc, 0xa3, 0x7e, 0x38, 0xc6, 0x37, 0x11, 0x4f, 0xed, 0xdd, 0x68, 0x31, 0x8e, 0x4c,
    0xd2, 0xb3, 0x01, 0x74, 0xee, 0xbe, 0x75, 0x5e, 0x07, 0x48, 0x1a, 0x7f, 0x70, 0xff, 0x16, 0x5c, 0x84, 0xc0, 0x79,
    0x85, 0xb8, 0x05, 0xfd, 0x7f, 0xbe, 0x65, 0x11, 0xa3, 0x0f, 0xc0, 0x02, 0xb4, 0xf8, 0x52, 0x37, 0x39, 0x04, 0xd5,
    0xa9, 0x31, 0x7a, 0x18, 0xbf, 0xa0, 0x2a, 0xf4, 0x12, 0x99, 0xf7, 0xa3, 0x45, 0x82, 0xe3, 0x3c, 0x5e, 0xf5, 0x9d,
    0x9e, 0xb5, 0xc8, 0x9e, 0x7c, 0x2e, 0xc8, 0xa4, 0x9e, 0x4e, 0x08, 0x14, 0x4b, 0x6d, 0xfd, 0x70, 0x6d, 0x6b, 0x1a,
    0x63, 0xbd, 0x64, 0xe6, 0x1f, 0xb7, 0xce, 0xf0, 0xf2, 0x9f, 0x2e, 0xbb, 0x1b, 0xb7, 0xf2, 0x50, 0x88, 0x73, 0x92,
    0xc2, 0xe2, 0xe3, 0x16, 0x8d, 0x9a, 0x32, 0x02, 0xab, 0x8e, 0x18, 0xdd, 0xe9, 0x10, 0x11, 0xee, 0x7e, 0x35, 0xab,
    0x90, 0xaf, 0x3e, 0x30, 0x94, 0x7a, 0xd0, 0x33, 0x3d, 0xa7, 0x65, 0x0f, 0xf5, 0xfc, 0x8e, 0x9e, 0x62, 0xcf, 0x47,
    0x44, 0x2c, 0x01, 0x5d, 0xbb, 0x1d, 0xb5, 0x32, 0xd2, 0x47, 0xd2, 0x38, 0x2e, 0xd0, 0xfe, 0x81, 0xdc, 0x32, 0x6a,
    0x1e, 0xb5, 0xee, 0x3c, 0xd5, 0xfc, 0xe7, 0x81, 0x1d, 0x19, 0xc3, 0x24, 0x42, 0xea, 0x63, 0x39, 0xa9};
const unsigned int _nx_azure_iot_root_cert_size = sizeof(_nx_azure_iot_root_cert);

/* Azure IoT Hub and DPS will migrate to the Digicert Global Root G2.
   To continue without disruption due to this changes, Microsoft recommends that
   client applications or devices trust the Digicert Global Root G2.  */

/* Digicert Global Root G2  */
const unsigned char _nx_azure_iot_root_cert_2[] = {
    0x30, 0x82, 0x03, 0x8e, 0x30, 0x82, 0x02, 0x76, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x03, 0x3a, 0xf1, 0xe6,
    0xa7, 0x11, 0xa9, 0xa0, 0xbb, 0x28, 0x64, 0xb1, 0x1d, 0x09, 0xfa, 0xe5, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
    0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30, 0x61, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06,
    0x13, 0x02, 0x55, 0x53, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0c, 0x44, 0x69, 0x67, 0x69,
    0x43, 0x65, 0x72, 0x74, 0x20, 0x49, 0x6e, 0x63, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x10,
    0x77, 0x77, 0x77, 0x2e, 0x64, 0x69, 0x67, 0x69, 0x63, 0x65, 0x72, 0x74, 0x2e, 0x63, 0x6f, 0x6d, 0x31, 0x20, 0x30,
    0x1e, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x17, 0x44, 0x69, 0x67, 0x69, 0x43, 0x65, 0x72, 0x74, 0x20, 0x47, 0x6c,
    0x6f, 0x62, 0x61, 0x6c, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x47, 0x32, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x33, 0x30,
    0x38, 0x30, 0x31, 0x31, 0x32, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x33, 0x38, 0x30, 0x31, 0x31, 0x35, 0x31,
    0x32, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x61, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
    0x55, 0x53, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0c, 0x44, 0x69, 0x67, 0x69, 0x43, 0x65,
    0x72, 0x74, 0x20, 0x49, 0x6e, 0x63, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x10, 0x77, 0x77,
    0x77, 0x2e, 0x64, 0x69, 0x67, 0x69, 0x63, 0x65, 0x72, 0x74, 0x2e, 0x63, 0x6f, 0x6d, 0x31, 0x20, 0x30, 0x1e, 0x06,
    0x03, 0x55, 0x04, 0x03, 0x13, 0x17, 0x44, 0x69, 0x67, 0x69, 0x43, 0x65, 0x72, 0x74, 0x20, 0x47, 0x6c, 0x6f, 0x62,
    0x61, 0x6c, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x47, 0x32, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a,
    0x02, 0x82, 0x01, 0x01, 0x00, 0xbb, 0x37, 0xcd, 0x34, 0xdc, 0x7b, 0x6b, 0xc9, 0xb2, 0x68, 0x90, 0xad, 0x4a, 0x75,
    0xff, 0x46, 0xba, 0x21, 0x0a, 0x08, 0x8d, 0xf5, 0x19, 0x54, 0xc9, 0xfb, 0x88, 0xdb, 0xf3, 0xae, 0xf2, 0x3a, 0x89,
    0x91, 0x3c, 0x7a, 0xe6, 0xab, 0x06, 0x1a, 0x6b, 0xcf, 0xac, 0x2d, 0xe8, 0x5e, 0x09, 0x24, 0x44, 0xba, 0x62, 0x9a,
    0x7e, 0xd6, 0xa3, 0xa8, 0x7e, 0xe0, 0x54, 0x75, 0x20, 0x05, 0xac, 0x50, 0xb7, 0x9c, 0x63, 0x1a, 0x6c, 0x30, 0xdc,
    0xda, 0x1f, 0x19, 0xb1, 0xd7, 0x1e, 0xde, 0xfd, 0xd7, 0xe0, 0xcb, 0x94, 0x83, 0x37, 0xae, 0xec, 0x1f, 0x43, 0x4e,
    0xdd, 0x7b, 0x2c, 0xd2, 0xbd, 0x2e, 0xa5, 0x2f, 0xe4, 0xa9, 0xb8, 0xad, 0x3a, 0xd4, 0x99, 0xa4, 0xb6, 0x25, 0xe9,
    0x9b, 0x6b, 0x00, 0x60, 0x92, 0x60, 0xff, 0x4f, 0x21, 0x49, 0x18, 0xf7, 0x67, 0x90, 0xab, 0x61, 0x06, 0x9c, 0x8f,
    0xf2, 0xba, 0xe9, 0xb4, 0xe9, 0x92, 0x32, 0x6b, 0xb5, 0xf3, 0x57, 0xe8, 0x5d, 0x1b, 0xcd, 0x8c, 0x1d, 0xab, 0x95,
    0x04, 0x95, 0x49, 0xf3, 0x35, 0x2d, 0x96, 0xe3, 0x49, 0x6d, 0xdd, 0x77, 0xe3, 0xfb, 0x49, 0x4b, 0xb4, 0xac, 0x55,
    0x07, 0xa9, 0x8f, 0x95, 0xb3, 0xb4, 0x23, 0xbb, 0x4c, 0x6d, 0x45, 0xf0, 0xf6, 0xa9, 0xb2, 0x95, 0x30, 0xb4, 0xfd,
    0x4c, 0x55, 0x8c, 0x27, 0x4a, 0x57, 0x14, 0x7c, 0x82, 0x9d, 0xcd, 0x73, 0x92, 0xd3, 0x16, 0x4a, 0x06, 0x0c, 0x8c,
    0x50, 0xd1, 0x8f, 0x1e, 0x09, 0xbe, 0x17, 0xa1, 0xe6, 0x21, 0xca, 0xfd, 0x83, 0xe5, 0x10, 0xbc, 0x83, 0xa5, 0x0a,
    0xc4, 0x67, 0x28, 0xf6, 0x73, 0x14, 0x14, 0x3d, 0x46, 0x76, 0xc3, 0x87, 0x14, 0x89, 0x21, 0x34, 0x4d, 0xaf, 0x0f,
    0x45, 0x0c, 0xa6, 0x49, 0xa1, 0xba, 0xbb, 0x9c, 0xc5, 0xb1, 0x33, 0x83, 0x29, 0x85, 0x02, 0x03, 0x01, 0x00, 0x01,
    0xa3, 0x42, 0x30, 0x40, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01,
    0x01, 0xff, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30,
    0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x4e, 0x22, 0x54, 0x20, 0x18, 0x95, 0xe6, 0xe3, 0x6e,
    0xe6, 0x0f, 0xfa, 0xfa, 0xb9, 0x12, 0xed, 0x06, 0x17, 0x8f, 0x39, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x60, 0x67, 0x28, 0x94, 0x6f, 0x0e, 0x48,
    0x63, 0xeb, 0x31, 0xdd, 0xea, 0x67, 0x18, 0xd5, 0x89, 0x7d, 0x3c, 0xc5, 0x8b, 0x4a, 0x7f, 0xe9, 0xbe, 0xdb, 0x2b,
    0x17, 0xdf, 0xb0, 0x5f, 0x73, 0x77, 0x2a, 0x32, 0x13, 0x39, 0x81, 0x67, 0x42, 0x84, 0x23, 0xf2, 0x45, 0x67, 0x35,
    0xec, 0x88, 0xbf, 0xf8, 0x8f, 0xb0, 0x61, 0x0c, 0x34, 0xa4, 0xae, 0x20, 0x4c, 0x84, 0xc6, 0xdb, 0xf8, 0x35, 0xe1,
    0x76, 0xd9, 0xdf, 0xa6, 0x42, 0xbb, 0xc7, 0x44, 0x08, 0x86, 0x7f, 0x36, 0x74, 0x24, 0x5a, 0xda, 0x6c, 0x0d, 0x14,
    0x59, 0x35, 0xbd, 0xf2, 0x49, 0xdd, 0xb6, 0x1f, 0xc9, 0xb3, 0x0d, 0x47, 0x2a, 0x3d, 0x99, 0x2f, 0xbb, 0x5c, 0xbb,
    0xb5, 0xd4, 0x20, 0xe1, 0x99, 0x5f, 0x53, 0x46, 0x15, 0xdb, 0x68, 0x9b, 0xf0, 0xf3, 0x30, 0xd5, 0x3e, 0x31, 0xe2,
    0x8d, 0x84, 0x9e, 0xe3, 0x8a, 0xda, 0xda, 0x96, 0x3e, 0x35, 0x13, 0xa5, 0x5f, 0xf0, 0xf9, 0x70, 0x50, 0x70, 0x47,
    0x41, 0x11, 0x57, 0x19, 0x4e, 0xc0, 0x8f, 0xae, 0x06, 0xc4, 0x95, 0x13, 0x17, 0x2f, 0x1b, 0x25, 0x9f, 0x75, 0xf2,
    0xb1, 0x8e, 0x99, 0xa1, 0x6f, 0x13, 0xb1, 0x41, 0x71, 0xfe, 0x88, 0x2a, 0xc8, 0x4f, 0x10, 0x20, 0x55, 0xd7, 0xf3,
    0x14, 0x45, 0xe5, 0xe0, 0x44, 0xf4, 0xea, 0x87, 0x95, 0x32, 0x93, 0x0e, 0xfe, 0x53, 0x46, 0xfa, 0x2c, 0x9d, 0xff,
    0x8b, 0x22, 0xb9, 0x4b, 0xd9, 0x09, 0x45, 0xa4, 0xde, 0xa4, 0xb8, 0x9a, 0x58, 0xdd, 0x1b, 0x7d, 0x52, 0x9f, 0x8e,
    0x59, 0x43, 0x88, 0x81, 0xa4, 0x9e, 0x26, 0xd5, 0x6f, 0xad, 0xdd, 0x0d, 0xc6, 0x37, 0x7d, 0xed, 0x03, 0x92, 0x1b,
    0xe5, 0x77, 0x5f, 0x76, 0xee, 0x3c, 0x8d, 0xc4, 0x5d, 0x56, 0x5b, 0xa2, 0xd9, 0x66, 0x6e, 0xb3, 0x35, 0x37, 0xe5,
    0x32, 0xb6};
const unsigned int _nx_azure_iot_root_cert_size_2 = sizeof(_nx_azure_iot_root_cert_2);

/* To prevent future disruption, client applications or devices should also add the following root.  */

/* Microsoft RSA Root Certificate Authority 2017  */
const unsigned char _nx_azure_iot_root_cert_3[] = {
    0x30, 0x82, 0x05, 0xa8, 0x30, 0x82, 0x03, 0x90, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x1e, 0xd3, 0x97, 0x09,
    0x5f, 0xd8, 0xb4, 0xb3, 0x47, 0x70, 0x1e, 0xaa, 0xbe, 0x7f, 0x45, 0xb3, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
    0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c, 0x05, 0x00, 0x30, 0x65, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06,
    0x13, 0x02, 0x55, 0x53, 0x31, 0x1e, 0x30, 0x1c, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x15, 0x4d, 0x69, 0x63, 0x72,
    0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x43, 0x6f, 0x72, 0x70, 0x6f, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x31, 0x36,
    0x30, 0x34, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x2d, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20,
    0x52, 0x53, 0x41, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74,
    0x65, 0x20, 0x41, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x20, 0x32, 0x30, 0x31, 0x37, 0x30, 0x1e, 0x17,
    0x0d, 0x31, 0x39, 0x31, 0x32, 0x31, 0x38, 0x32, 0x32, 0x35, 0x31, 0x32, 0x32, 0x5a, 0x17, 0x0d, 0x34, 0x32, 0x30,
    0x37, 0x31, 0x38, 0x32, 0x33, 0x30, 0x30, 0x32, 0x33, 0x5a, 0x30, 0x65, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
    0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x1e, 0x30, 0x1c, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x15, 0x4d, 0x69,
    0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x43, 0x6f, 0x72, 0x70, 0x6f, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e,
    0x31, 0x36, 0x30, 0x34, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x2d, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66,
    0x74, 0x20, 0x52, 0x53, 0x41, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63,
    0x61, 0x74, 0x65, 0x20, 0x41, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x20, 0x32, 0x30, 0x31, 0x37, 0x30,
    0x82, 0x02, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03,
    0x82, 0x02, 0x0f, 0x00, 0x30, 0x82, 0x02, 0x0a, 0x02, 0x82, 0x02, 0x01, 0x00, 0xca, 0x5b, 0xbe, 0x94, 0x33, 0x8c,
    0x29, 0x95, 0x91, 0x16, 0x0a, 0x95, 0xbd, 0x47, 0x62, 0xc1, 0x89, 0xf3, 0x99, 0x36, 0xdf, 0x46, 0x90, 0xc9, 0xa5,
    0xed, 0x78, 0x6a, 0x6f, 0x47, 0x91, 0x68, 0xf8, 0x27, 0x67, 0x50, 0x33, 0x1d, 0xa1, 0xa6, 0xfb, 0xe0, 0xe5, 0x43,
    0xa3, 0x84, 0x02, 0x57, 0x01, 0x5d, 0x9c, 0x48, 0x40, 0x82, 0x53, 0x10, 0xbc, 0xbf, 0xc7, 0x3b, 0x68, 0x90, 0xb6,
    0x82, 0x2d, 0xe5, 0xf4, 0x65, 0xd0, 0xcc, 0x6d, 0x19, 0xcc, 0x95, 0xf9, 0x7b, 0xac, 0x4a, 0x94, 0xad, 0x0e, 0xde,
    0x4b, 0x43, 0x1d, 0x87, 0x07, 0x92, 0x13, 0x90, 0x80, 0x83, 0x64, 0x35, 0x39, 0x04, 0xfc, 0xe5, 0xe9, 0x6c, 0xb3,
    0xb6, 0x1f, 0x50, 0x94, 0x38, 0x65, 0x50, 0x5c, 0x17, 0x46, 0xb9, 0xb6, 0x85, 0xb5, 0x1c, 0xb5, 0x17, 0xe8, 0xd6,
    0x45, 0x9d, 0xd8, 0xb2, 0x26, 0xb0, 0xca, 0xc4, 0x70, 0x4a, 0xae, 0x60, 0xa4, 0xdd, 0xb3, 0xd9, 0xec, 0xfc, 0x3b,
    0xd5, 0x57, 0x72, 0xbc, 0x3f, 0xc8, 0xc9, 0xb2, 0xde, 0x4b, 0x6b, 0xf8, 0x23, 0x6c, 0x03, 0xc0, 0x05, 0xbd, 0x95,
    0xc7, 0xcd, 0x73, 0x3b, 0x66, 0x80, 0x64, 0xe3, 0x1a, 0xac, 0x2e, 0xf9, 0x47, 0x05, 0xf2, 0x06, 0xb6, 0x9b, 0x73,
    0xf5, 0x78, 0x33, 0x5b, 0xc7, 0xa1, 0xfb, 0x27, 0x2a, 0xa1, 0xb4, 0x9a, 0x91, 0x8c, 0x91, 0xd3, 0x3a, 0x82, 0x3e,
    0x76, 0x40, 0xb4, 0xcd, 0x52, 0x61, 0x51, 0x70, 0x28, 0x3f, 0xc5, 0xc5, 0x5a, 0xf2, 0xc9, 0x8c, 0x49, 0xbb, 0x14,
    0x5b, 0x4d, 0xc8, 0xff, 0x67, 0x4d, 0x4c, 0x12, 0x96, 0xad, 0xf5, 0xfe, 0x78, 0xa8, 0x97, 0x87, 0xd7, 0xfd, 0x5e,
    0x20, 0x80, 0xdc, 0xa1, 0x4b, 0x22, 0xfb, 0xd4, 0x89, 0xad, 0xba, 0xce, 0x47, 0x97, 0x47, 0x55, 0x7b, 0x8f, 0x45,
    0xc8, 0x67, 0x28, 0x84, 0x95, 0x1c, 0x68, 0x30, 0xef, 0xef, 0x49, 0xe0, 0x35, 0x7b, 0x64, 0xe7, 0x98, 0xb0, 0x94,
    0xda, 0x4d, 0x85, 0x3b, 0x3e, 0x55, 0xc4, 0x28, 0xaf, 0x57, 0xf3, 0x9e, 0x13, 0xdb, 0x46, 0x27, 0x9f, 0x1e, 0xa2,
    0x5e, 0x44, 0x83, 0xa4, 0xa5, 0xca, 0xd5, 0x13, 0xb3, 0x4b, 0x3f, 0xc4, 0xe3, 0xc2, 0xe6, 0x86, 0x61, 0xa4, 0x52,
    0x30, 0xb9, 0x7a, 0x20, 0x4f, 0x6f, 0x0f, 0x38, 0x53, 0xcb, 0x33, 0x0c, 0x13, 0x2b, 0x8f, 0xd6, 0x9a, 0xbd, 0x2a,
    0xc8, 0x2d, 0xb1, 0x1c, 0x7d, 0x4b, 0x51, 0xca, 0x47, 0xd1, 0x48, 0x27, 0x72, 0x5d, 0x87, 0xeb, 0xd5, 0x45, 0xe6,
    0x48, 0x65, 0x9d, 0xaf, 0x52, 0x90, 0xba, 0x5b, 0xa2, 0x18, 0x65, 0x57, 0x12, 0x9f, 0x68, 0xb9, 0xd4, 0x15, 0x6b,
    0x94, 0xc4, 0x69, 0x22, 0x98, 0xf4, 0x33, 0xe0, 0xed, 0xf9, 0x51, 0x8e, 0x41, 0x50, 0xc9, 0x34, 0x4f, 0x76, 0x90,
    0xac, 0xfc, 0x38, 0xc1, 0xd8, 0xe1, 0x7b, 0xb9, 0xe3, 0xe3, 0x94, 0xe1, 0x46, 0x69, 0xcb, 0x0e, 0x0a, 0x50, 0x6b,
    0x13, 0xba, 0xac, 0x0f, 0x37, 0x5a, 0xb7, 0x12, 0xb5, 0x90, 0x81, 0x1e, 0x56, 0xae, 0x57, 0x22, 0x86, 0xd9, 0xc9,
    0xd2, 0xd1, 0xd7, 0x51, 0xe3, 0xab, 0x3b, 0xc6, 0x55, 0xfd, 0x1e, 0x0e, 0xd3, 0x74, 0x0a, 0xd1, 0xda, 0xaa, 0xea,
    0x69, 0xb8, 0x97, 0x28, 0x8f, 0x48, 0xc4, 0x07, 0xf8, 0x52, 0x43, 0x3a, 0xf4, 0xca, 0x55, 0x35, 0x2c, 0xb0, 0xa6,
    0x6a, 0xc0, 0x9c, 0xf9, 0xf2, 0x81, 0xe1, 0x12, 0x6a, 0xc0, 0x45, 0xd9, 0x67, 0xb3, 0xce, 0xff, 0x23, 0xa2, 0x89,
    0x0a, 0x54, 0xd4, 0x14, 0xb9, 0x2a, 0xa8, 0xd7, 0xec, 0xf9, 0xab, 0xcd, 0x25, 0x58, 0x32, 0x79, 0x8f, 0x90, 0x5b,
    0x98, 0x39, 0xc4, 0x08, 0x06, 0xc1, 0xac, 0x7f, 0x0e, 0x3d, 0x00, 0xa5, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x54,
    0x30, 0x52, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30,
    0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x1d, 0x06,
    0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x09, 0xcb, 0x59, 0x7f, 0x86, 0xb2, 0x70, 0x8f, 0x1a, 0xc3, 0x39,
    0xe3, 0xc0, 0xd9, 0xe9, 0xbf, 0xbb, 0x4d, 0xb2, 0x23, 0x30, 0x10, 0x06, 0x09, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82,
    0x37, 0x15, 0x01, 0x04, 0x03, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
    0x01, 0x0c, 0x05, 0x00, 0x03, 0x82, 0x02, 0x01, 0x00, 0xac, 0xaf, 0x3e, 0x5d, 0xc2, 0x11, 0x96, 0x89, 0x8e, 0xa3,
    0xe7, 0x92, 0xd6, 0x97, 0x15, 0xb8, 0x13, 0xa2, 0xa6, 0x42, 0x2e, 0x02, 0xcd, 0x16, 0x05, 0x59, 0x27, 0xca, 0x20,
    0xe8, 0xba, 0xb8, 0xe8, 0x1a, 0xec, 0x4d, 0xa8, 0x97, 0x56, 0xae, 0x65, 0x43, 0xb1, 0x8f, 0x00, 0x9b, 0x52, 0xcd,
    0x55, 0xcd, 0x53, 0x39, 0x6d, 0x62, 0x4c, 0x8b, 0x0d, 0x5b, 0x7c, 0x2e, 0x44, 0xbf, 0x83, 0x10, 0x8f, 0xf3, 0x53,
    0x82, 0x80, 0xc3, 0x4f, 0x3a, 0xc7, 0x6e, 0x11, 0x3f, 0xe6, 0xe3, 0x16, 0x91, 0x84, 0xfb, 0x6d, 0x84, 0x7f, 0x34,
    0x74, 0xad, 0x89, 0xa7, 0xce, 0xb9, 0xd7, 0xd7, 0x9f, 0x84, 0x64, 0x92, 0xbe, 0x95, 0xa1, 0xad, 0x09, 0x53, 0x33,
    0xdd, 0xee, 0x0a, 0xea, 0x4a, 0x51, 0x8e, 0x6f, 0x55, 0xab, 0xba, 0xb5, 0x94, 0x46, 0xae, 0x8c, 0x7f, 0xd8, 0xa2,
    0x50, 0x25, 0x65, 0x60, 0x80, 0x46, 0xdb, 0x33, 0x04, 0xae, 0x6c, 0xb5, 0x98, 0x74, 0x54, 0x25, 0xdc, 0x93, 0xe4,
    0xf8, 0xe3, 0x55, 0x15, 0x3d, 0xb8, 0x6d, 0xc3, 0x0a, 0xa4, 0x12, 0xc1, 0x69, 0x85, 0x6e, 0xdf, 0x64, 0xf1, 0x53,
    0x99, 0xe1, 0x4a, 0x75, 0x20, 0x9d, 0x95, 0x0f, 0xe4, 0xd6, 0xdc, 0x03, 0xf1, 0x59, 0x18, 0xe8, 0x47, 0x89, 0xb2,
    0x57, 0x5a, 0x94, 0xb6, 0xa9, 0xd8, 0x17, 0x2b, 0x17, 0x49, 0xe5, 0x76, 0xcb, 0xc1, 0x56, 0x99, 0x3a, 0x37, 0xb1,
    0xff, 0x69, 0x2c, 0x91, 0x91, 0x93, 0xe1, 0xdf, 0x4c, 0xa3, 0x37, 0x76, 0x4d, 0xa1, 0x9f, 0xf8, 0x6d, 0x1e, 0x1d,
    0xd3, 0xfa, 0xec, 0xfb, 0xf4, 0x45, 0x1d, 0x13, 0x6d, 0xcf, 0xf7, 0x59, 0xe5, 0x22, 0x27, 0x72, 0x2b, 0x86, 0xf3,
    0x57, 0xbb, 0x30, 0xed, 0x24, 0x4d, 0xdc, 0x7d, 0x56, 0xbb, 0xa3, 0xb3, 0xf8, 0x34, 0x79, 0x89, 0xc1, 0xe0, 0xf2,
    0x02, 0x61, 0xf7, 0xa6, 0xfc, 0x0f, 0xbb, 0x1c, 0x17, 0x0b, 0xae, 0x41, 0xd9, 0x7c, 0xbd, 0x27, 0xa3, 0xfd, 0x2e,
    0x3a, 0xd1, 0x93, 0x94, 0xb1, 0x73, 0x1d, 0x24, 0x8b, 0xaf, 0x5b, 0x20, 0x89, 0xad, 0xb7, 0x67, 0x66, 0x79, 0xf5,
    0x3a, 0xc6, 0xa6, 0x96, 0x33, 0xfe, 0x53, 0x92, 0xc8, 0x46, 0xb1, 0x11, 0x91, 0xc6, 0x99, 0x7f, 0x8f, 0xc9, 0xd6,
    0x66, 0x31, 0x20, 0x41, 0x10, 0x87, 0x2d, 0x0c, 0xd6, 0xc1, 0xaf, 0x34, 0x98, 0xca, 0x64, 0x83, 0xfb, 0x13, 0x57,
    0xd1, 0xc1, 0xf0, 0x3c, 0x7a, 0x8c, 0xa5, 0xc1, 0xfd, 0x95, 0x21, 0xa0, 0x71, 0xc1, 0x93, 0x67, 0x71, 0x12, 0xea,
    0x8f, 0x88, 0x0a, 0x69, 0x19, 0x64, 0x99, 0x23, 0x56, 0xfb, 0xac, 0x2a, 0x2e, 0x70, 0xbe, 0x66, 0xc4, 0x0c, 0x84,
    0xef, 0xe5, 0x8b, 0xf3, 0x93, 0x01, 0xf8, 0x6a, 0x90, 0x93, 0x67, 0x4b, 0xb2, 0x68, 0xa3, 0xb5, 0x62, 0x8f, 0xe9,
    0x3f, 0x8c, 0x7a, 0x3b, 0x5e, 0x0f, 0xe7, 0x8c, 0xb8, 0xc6, 0x7c, 0xef, 0x37, 0xfd, 0x74, 0xe2, 0xc8, 0x4f, 0x33,
    0x72, 0xe1, 0x94, 0x39, 0x6d, 0xbd, 0x12, 0xaf, 0xbe, 0x0c, 0x4e, 0x70, 0x7c, 0x1b, 0x6f, 0x8d, 0xb3, 0x32, 0x93,
    0x73, 0x44, 0x16, 0x6d, 0xe8, 0xf4, 0xf7, 0xe0, 0x95, 0x80, 0x8f, 0x96, 0x5d, 0x38, 0xa4, 0xf4, 0xab, 0xde, 0x0a,
    0x30, 0x87, 0x93, 0xd8, 0x4d, 0x00, 0x71, 0x62, 0x45, 0x27, 0x4b, 0x3a, 0x42, 0x84, 0x5b, 0x7f, 0x65, 0xb7, 0x67,
    0x34, 0x52, 0x2d, 0x9c, 0x16, 0x6b, 0xaa, 0xa8, 0xd8, 0x7b, 0xa3, 0x42, 0x4c, 0x71, 0xc7, 0x0c, 0xca, 0x3e, 0x83,
    0xe4, 0xa6, 0xef, 0xb7, 0x01, 0x30, 0x5e, 0x51, 0xa3, 0x79, 0xf5, 0x70, 0x69, 0xa6, 0x41, 0x44, 0x0f, 0x86, 0xb0,
    0x2c, 0x91, 0xc6, 0x3d, 0xea, 0xae, 0x0f, 0x84};
const unsigned int _nx_azure_iot_root_cert_size_3 = sizeof(_nx_azure_iot_root_cert_3);
