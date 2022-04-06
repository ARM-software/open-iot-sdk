/*
 * Copyright (c) 2021 Arm Limited
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

#include "stdio.h"
#include <string.h>
#include "bsp_serial.h"
#include "print_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"

/* includes for TFM */
#include "tfm_ns_interface.h"
#include "psa/protected_storage.h"
#include "psa/crypto.h"


/* -----------------------------------------------------------------------------
 *  Helper functions
 * -----------------------------------------------------------------------------
 */

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    print_log("Assert called %s:%lu", pcFile, ulLine);
    while(1);
}


/**
 * This function is only used in the PKCS#11 test case. In the PKCS#11 test,
 * it calls the mbedtls steps to generate the random number, so this function
 * is needed. But in the PKCS#11 library, we call the C_GenerateRandom to
 * generate a random number and do not need to call this function.
 */
int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    ( void ) (data);
    ( void ) (len);

    static uint32_t random_number = 0;

    random_number += 8;
    memcpy(output, &random_number, sizeof(uint32_t));
    *olen = sizeof(uint32_t);

    return 0;
}
