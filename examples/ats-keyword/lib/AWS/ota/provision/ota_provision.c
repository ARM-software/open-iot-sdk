/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
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

/* Key provisioning include. */
#include "ota_provision.h"

/* This is the public key which is derivated from ./bl2/ext/mcuboot/root-rsa-2048_1.pem.
 * If you used a different key to sign the image, then please replace the values here
 * with your public key. Also please note that the OTA service only support RSA2048(RSA3072
 * is not supported). */
static const char cOTARSAPublicKey[] =
	"-----BEGIN PUBLIC KEY-----\n"
	"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArNJ0kz5f56/yyGzoWFFj\n"
	"dw5S/ljVu6XjnIrNFAqJxhWuSQSfXz0riRIfPn8F/AuZJmNelu8zvIwnaE1GjWYz\n"
	"mTs4ERvjrBgHlUiHsqtPLQ4SURt/M7p4sdX6f79xS+RfZ1dn1au7ZAYXPYHr2MH5\n"
	"elfTKVwQ/6fTOlg/JYrFhHuXJ6XkkOffHDPmfK9od14fCW7dkmBOrHOEsPe2AsLO\n"
	"n6+tsrFXzPkGHWolL3Iqff4N7bjClYhB8kWobmqF7q76inn6/n5ASUPsLI6Ogn7i\n"
	"+A/y6X2jf6wjvQpC6hj7cqCaJAHIJ4xWJJOC3yMZlnPyEcMF5qW4C+BzzgebV+aO\n"
	"+wIDAQAB\n"
	"-----END PUBLIC KEY-----";

/* This function can be found in libraries/3rdparty/mbedtls_utils/mbedtls_utils.c. */
extern int convert_pem_to_der( const unsigned char * pucInput,
                               size_t xLen,
                               unsigned char * pucOutput,
                               size_t * pxOlen );

int ota_privision_code_signing_key(psa_key_handle_t * key_handle)
{
    uint8_t public_key_der[310];
    size_t xLength = 310;
    int result;
    psa_key_handle_t key_handle_tmp = NULL;
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    result = convert_pem_to_der( ( const unsigned char * ) cOTARSAPublicKey,
                                            sizeof( cOTARSAPublicKey ),
                                            public_key_der,
                                            &xLength );
    if( result != 0 )
    {
        return result;
    }

    psa_set_key_usage_flags( &attributes, PSA_KEY_USAGE_VERIFY_HASH );
    psa_set_key_algorithm( &attributes, PSA_ALG_RSA_PSS_ANY_SALT( PSA_ALG_SHA_256 ) );
    psa_set_key_type( &attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY );
    psa_set_key_bits(&attributes, 2048);
    status = psa_import_key(&attributes, ( const uint8_t *)public_key_der, xLength, &key_handle_tmp );
    if( status == PSA_SUCCESS )
    {
        *key_handle = key_handle_tmp;
    }
    return status;
}
