/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stdio.h"
#include <string.h>
#include "bsp_serial.h"
#include "print_log.h"
#include "cmsis_os2.h" 
#include "RTOS_config.h"
/* includes for TFM */
#include "tfm_ns_interface.h"
#include "psa/protected_storage.h"
#include "psa/crypto.h"
#include "emac_cs300.h"

/* Provide EMAC interface to LwIP. This is required until MDH provides a factory
 * function for it.
 */
mdh_emac_t *lwip_emac_get_default_instance(void)
{
    return cs300_emac_get_default_instance();
}

/* -----------------------------------------------------------------------------
 *  Helper declarations
 * -----------------------------------------------------------------------------
 */
/**
 * Freertos heap declaration. It's required when configAPPLICATION_ALLOCATED_HEAP i set.
 */
#if (configAPPLICATION_ALLOCATED_HEAP == 1)
    uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* -----------------------------------------------------------------------------
 *  Helper functions
 * -----------------------------------------------------------------------------
 */

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
    print_log("Assert failed %s:%lu", pcFile, ulLine);
    while(1);
}

void configASSERT(uint32_t condition)
{
    if ( condition == 0 ) {
        vAssertCalled( __FILE__, __LINE__ );
    }
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
