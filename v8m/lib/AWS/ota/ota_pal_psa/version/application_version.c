/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file application_version.c
 * @brief Get the image version which is used by the AWS OTA
 *
 */

/* C Runtime includes. */
#include <stdlib.h>
#include "application_version.h"

AppVersion32_t xAppFirmwareVersion;

int GetImageVersionPSA( psa_fwu_component_t uxComponent )
{
    psa_fwu_component_info_t xComponentInfo = { 0 };
    psa_status_t uxStatus;

    uxStatus = psa_fwu_query( uxComponent, &xComponentInfo );
    if( uxStatus == PSA_SUCCESS )
    {
        xAppFirmwareVersion.u.x.major = xComponentInfo.version.major;
        xAppFirmwareVersion.u.x.minor = xComponentInfo.version.minor;
        xAppFirmwareVersion.u.x.build = (uint16_t)xComponentInfo.version.patch;
        return 0;
    }
    else
    {
        xAppFirmwareVersion.u.signedVersion32 = 0;
        return -1;
    }
}
