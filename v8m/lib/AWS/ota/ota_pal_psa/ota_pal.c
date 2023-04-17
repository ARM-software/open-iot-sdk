/*
 * AWS IoT Over-the-air Update v3.0.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2021-2023 Arm Limited. All rights reserved.
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
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file ota_pal.c
 * @brief Platform Abstraction layer for AWS OTA based on PSA API
 *
 */

#include <string.h>


#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */

#define LIBRARY_LOG_NAME    "ota_pal"
#define LIBRARY_LOG_LEVEL    LOG_INFO

#include "logging_stack.h"

/* To provide appFirmwareVersion for OTA library. */
#include "ota_appversion32.h"

/* OTA PAL Port include. */
#include "ota_pal.h"

/* PSA services. */
#include "psa/update.h"
#include "psa/crypto.h"

/***********************************************************************
 *
 * Macros
 *
 **********************************************************************/

#define ECDSA_SHA256_RAW_SIGNATURE_LENGTH     ( 64 )

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/***********************************************************************
 *
 * Variables
 *
 **********************************************************************/
 /**
 * @brief File Signature Key
 *
 * The OTA signature algorithm we support on this platform.
 */
#if defined( OTA_PAL_CODE_SIGNING_ALGO ) && ( OTA_PAL_CODE_SIGNING_ALGO == OTA_PAL_CODE_SIGNING_RSA )
    const char OTA_JsonFileSignatureKey[ OTA_FILE_SIG_KEY_STR_MAX_LENGTH ] = "sig-sha256-rsa";
#else
    /* Use ECDSA as default if OTA_PAL_CODE_SIGNING_ALGO is not defined. */
    const char OTA_JsonFileSignatureKey[ OTA_FILE_SIG_KEY_STR_MAX_LENGTH ] = "sig-sha256-ecdsa";
#endif /* defined( OTA_PAL_CODE_SIGNING_ALGO ) && ( OTA_PAL_CODE_SIGNING_ALGO == OTA_PAL_CODE_SIGNING_RSA ) */


/**
 * @brief Ptr to system context
 *
 * Keep track of system context between calls from the OTA Agent
 *
 */
const OtaFileContext_t * pxSystemContext = NULL;
static psa_fwu_component_t xOTAComponentID = FWU_COMPONENT_NUMBER;

/* The key handle for OTA image verification. The key should be provisioned
 * before starting an OTA process by the user.
 */
extern psa_key_handle_t xOTACodeVerifyKeyHandle;

#if !defined( OTA_PAL_CODE_SIGNING_ALGO ) || ( OTA_PAL_CODE_SIGNING_ALGO == OTA_PAL_CODE_SIGNING_ECDSA )
    static uint8_t ucECDSARAWSignature[ ECDSA_SHA256_RAW_SIGNATURE_LENGTH ] = { 0 };
#endif /* !defined( OTA_PAL_CODE_SIGNING_ALGO ) || ( OTA_PAL_CODE_SIGNING_ALGO == OTA_PAL_CODE_SIGNING_ECDSA ) */

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

static bool prvConvertToRawECDSASignature( const uint8_t * pucEncodedSignature,  uint8_t * pucRawSignature )
{
    bool xReturn = true;
    const uint8_t * pxNextLength = NULL;
    uint8_t ucSigComponentLength;

    if( ( pucRawSignature == NULL ) || ( pucEncodedSignature == NULL ) )
    {
        xReturn = false;
    }

    if( xReturn == true )
    {
        /*
         * The signature has the ASN1-DER format:
         * SEQUENCE identifier: 8 bits
         * LENGTH: 8 bits (of entire rest of signature)
         * R_INTEGER identifier: 8 bits (of R component)
         * R_LENGTH: 8 bits (of R component)
         * R_VALUE: R_LENGTH (of R component)
         * S_INTEGER identifier: 8 bits (of S component)
         * S_LENGTH: 8 bits (of S component)
         * S_VALUE: S_LENGTH (of S component)
         */

        /* The 4th byte contains the length of the R component */
        ucSigComponentLength = pucEncodedSignature[ 3 ];

        /* The new signature will be 64 bytes long (32 bytes for R, 32 bytes for S).
         * Zero this buffer out in case a component is shorter than 32 bytes. */
        ( void ) memset( pucRawSignature, 0, ECDSA_SHA256_RAW_SIGNATURE_LENGTH );

        /********* R Component. *********/

        /* R components are represented by mbedTLS as 33 bytes when the first bit is zero to avoid any sign confusion. */
        if( ucSigComponentLength == 33UL )
        {
            /* Chop off the leading zero.  The first 4 bytes were SEQUENCE, LENGTH, R_INTEGER, R_LENGTH, 0x00 padding.  */
            ( void ) memcpy( pucRawSignature, &pucEncodedSignature[ 5 ], 32 );
            /* SEQUENCE, LENGTH, R_INTEGER, R_LENGTH, leading zero, R, S's integer tag */
            pxNextLength = &pucEncodedSignature[ 5U + 32U + 1U ];
        }
        else if( ucSigComponentLength <= 32UL )
        {
            /* The R component is 32 bytes or less.  Copy so that it is properly represented as a 32 byte value,
             * leaving leading 0 pads at beginning if necessary. */
            ( void ) memcpy( &pucRawSignature[ 32UL - ucSigComponentLength ],  /* If the R component is less than 32 bytes, leave the leading zeros. */
                             &pucEncodedSignature[ 4 ],                        /* SEQUENCE, LENGTH, R_INTEGER, R_LENGTH, (R component begins as the 5th byte) */
                             ucSigComponentLength );
            pxNextLength = &pucEncodedSignature[ 4U + ucSigComponentLength + 1U ]; /* Move the pointer to get rid of
                                                                                    * SEQUENCE, LENGTH, R_INTEGER, R_LENGTH, R Component, S_INTEGER tag. */
        }
        else
        {
            xReturn = false;
        }

        /********** S Component. ***********/

        if( xReturn == true )
        {
            /* Now pxNextLength is pointing to the length of the S component. */
            ucSigComponentLength = pxNextLength[ 0 ];

            if( ucSigComponentLength == 33UL )
            {
                ( void ) memcpy( &pucRawSignature[ 32 ],
                                 &pxNextLength[ 2 ], /* S_LENGTH (of S component), 0x00 padding, S component is 3rd byte - we want to skip the leading zero. */
                                 32 );
            }
            else if( ucSigComponentLength <= 32UL )
            {
                /* The S component is 32 bytes or less.  Copy so that it is properly represented as a 32 byte value,
                 * leaving leading 0 pads at beginning if necessary. */
                ( void ) memcpy( &pucRawSignature[ 64UL - ucSigComponentLength ],
                                 &pxNextLength[ 1 ],
                                 ucSigComponentLength );
            }
            else
            {
                xReturn = false;
            }
        }
    }

    return xReturn;
}

static OtaPalStatus_t PortConvertFilePathtoPSAComponentID ( OtaFileContext_t * const pFileContext,
                                                            psa_fwu_component_t * pxComponent )
{
    if( pFileContext == NULL || pxComponent == NULL || pFileContext->pFilePath == NULL )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalUninitialized, 0 );
    }

#ifdef FWU_COMPONENT_ID_SECURE
    /* pFilePath field is got from the OTA server. */
    if( memcmp( pFileContext->pFilePath, "secure image", strlen("secure image") ) == 0 )
    {
        *pxComponent = FWU_COMPONENT_ID_SECURE;
        return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
    }
#endif
#ifdef FWU_COMPONENT_ID_NONSECURE
    if( memcmp( pFileContext->pFilePath, "non_secure image", strlen("non_secure image") ) == 0 )
    {
        *pxComponent = FWU_COMPONENT_ID_NONSECURE;
        return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
    }
#endif
#ifdef FWU_COMPONENT_ID_FULL
    if( memcmp( pFileContext->pFilePath, "combined image", strlen("combined image") ) == 0 )
    {
        *pxComponent = FWU_COMPONENT_ID_FULL;
        return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
    }
#endif

    return OTA_PAL_COMBINE_ERR( OtaPalRxFileCreateFailed, 0 );
}

/**
 * @brief Abort an OTA transfer.
 *
 * Aborts access to an existing open file represented by the OTA file context pFileContext. This is
 * only valid for jobs that started successfully.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 *
 * This function may be called before the file is opened, so the file pointer pFileContext->fileHandle
 * may be NULL when this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess: Aborting access to the open file was successful.
 *   OtaPalFileAbort: Aborting access to the open file context was unsuccessful.
 */
OtaPalStatus_t otaPal_Abort( OtaFileContext_t * const pFileContext )
{
    OtaPalStatus_t retStatus = OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );

    if( ( pFileContext == NULL ) || ( ( pFileContext != pxSystemContext ) && ( pxSystemContext != NULL ) ) )
    {
        LogWarn( ( "otaPal_Abort: pFileContext or pFileContext->pFile is NULL." ) );
        retStatus = OTA_PAL_COMBINE_ERR( OtaPalAbortFailed, 0 );
    }
    else if( pFileContext->pFile == NULL )
    {
        /* Nothing to do. No open file associated with this context. */
    }
    else if( ( pFileContext != pxSystemContext ) && ( pxSystemContext != NULL ) )
    {
        LogWarn( ( "otaPal_Abort: pFileContext is different from pxSystemContext." ) );
        retStatus = OTA_PAL_COMBINE_ERR( OtaPalAbortFailed, 0 );

        pFileContext->pFile = NULL;
    }
    else if( pxSystemContext == NULL )
    {
        LogWarn( ( "otaPal_Abort: pxSystemContext is NULL." ) );
    }
    else
    {
        psa_status_t lPsaStatus;
        if( psa_fwu_cancel( xOTAComponentID ) != PSA_SUCCESS )
        {
            lPsaStatus = OTA_PAL_COMBINE_ERR( OtaPalAbortFailed, 0 );
        }
        if( psa_fwu_clean( xOTAComponentID ) != PSA_SUCCESS )
        {
            lPsaStatus = OTA_PAL_COMBINE_ERR( OtaPalAbortFailed, 0 );
        }
        /* psa_fwu_abort returns PSA_ERROR_INVALID_ARGUMENT if xOTAImageID was NOT written before abort.
         * But we should return success if xOTAImageID was created. */
        if( ( lPsaStatus != PSA_SUCCESS ) && ( lPsaStatus != PSA_ERROR_INVALID_ARGUMENT ) )
        {
            LogWarn( ( "otaPal_Abort: psa_fwu_abort fail with error %d.", lPsaStatus ) );
            retStatus = OTA_PAL_COMBINE_ERR( OtaPalAbortFailed, 1 );
        }

        pxSystemContext = NULL;
        xOTAComponentID = 0;
        pFileContext->pFile = NULL;
    }

    return retStatus;
}

/**
 * @brief Create a new receive file.
 *
 * @note Opens the file indicated in the OTA file context in the MCU file system.
 *
 * @note The previous image may be present in the designated image download partition or file, so the
 * partition or file must be completely erased or overwritten in this routine.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The device file path is a required field in the OTA job document, so pFileContext->pFilePath is
 * checked for NULL by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess: File creation was successful.
 *   OtaPalRxFileTooLarge: The OTA receive file is too big for the platform to support.
 *   OtaPalBootInfoCreateFailed: The bootloader information file creation failed.
 *   OtaPalRxFileCreateFailed: Returned for other errors creating the file in the device's
 *                             non-volatile memory. If this error is returned, then the sub error
 *                             should be set to the appropriate platform specific value.
 */
OtaPalStatus_t otaPal_CreateFileForRx( OtaFileContext_t * const pFileContext )
{
    psa_fwu_component_t uxComponent;

    if( pFileContext == NULL || pFileContext->pFilePath == NULL )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalRxFileCreateFailed, 0 );
    }

    if( PortConvertFilePathtoPSAComponentID( pFileContext, &uxComponent ) != OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 ) )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalRxFileCreateFailed, 0 );
    }

    /* Trigger a FWU process. Image manifest is bundled within the image. */
    if( psa_fwu_start( uxComponent, NULL, 0 ) != PSA_SUCCESS )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalRxFileCreateFailed, 0 );
    }

    pxSystemContext = pFileContext;
    xOTAComponentID = uxComponent;
    pFileContext->pFile = &xOTAComponentID;
    return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
}

static OtaPalStatus_t otaPal_CheckSignature( OtaFileContext_t * const pFileContext )
{
    psa_fwu_component_info_t xComponentInfo = { 0 };
    psa_status_t uxStatus;
    psa_key_attributes_t xKeyAttribute = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t xKeyAlgorithm = 0;
    uint8_t *ucSigBuffer = NULL;
    uint16_t usSigLength = 0;

    uxStatus = psa_fwu_query( xOTAComponentID, &xComponentInfo );
    if( uxStatus != PSA_SUCCESS )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalSignatureCheckFailed, OTA_PAL_SUB_ERR( uxStatus ) );
    }


#if ( defined( OTA_PAL_SIGNATURE_FORMAT ) && ( OTA_PAL_SIGNATURE_FORMAT == OTA_PAL_SIGNATURE_ASN1_DER ) )
    if( prvConvertToRawECDSASignature( pFileContext->pSignature->data,  ucECDSARAWSignature ) == false )
    {
        LogError( ( "Failed to decode ECDSA SHA256 signature." ) );
        return OTA_PAL_COMBINE_ERR( OtaPalSignatureCheckFailed, 0 );
    }

    ucSigBuffer = &ucECDSARAWSignature;
    usSigLength = ECDSA_SHA256_RAW_SIGNATURE_LENGTH;
#else
    ucSigBuffer = &pFileContext->pSignature->data;
    usSigLength = pFileContext->pSignature->size;
#endif /* defined( OTA_PAL_SIGNATURE_FORMAT ) && ( OTA_PAL_SIGNATURE_FORMAT == OTA_PAL_SIGNATURE_ASN1_DER ) */

    uxStatus = psa_get_key_attributes( xOTACodeVerifyKeyHandle, &xKeyAttribute );
    if( uxStatus != PSA_SUCCESS )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalSignatureCheckFailed, OTA_PAL_SUB_ERR( uxStatus ) );
    }

    xKeyAlgorithm = psa_get_key_algorithm( &xKeyAttribute );
    uxStatus = psa_verify_hash( xOTACodeVerifyKeyHandle,
                                xKeyAlgorithm,
                                ( const uint8_t * )xComponentInfo.impl.candidate_digest,
                                ( size_t )TFM_FWU_MAX_DIGEST_SIZE,
                                ucSigBuffer,
                                usSigLength );

    if( uxStatus != PSA_SUCCESS )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalSignatureCheckFailed, OTA_PAL_SUB_ERR( uxStatus ) );
    }

    return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
}

/**
 * @brief Authenticate and close the underlying receive file in the specified OTA context.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called. This function is called only at the end of block ingestion.
 * otaPAL_CreateFileForRx() must succeed before this function is reached, so
 * pFileContext->fileHandle(or pFileContext->pFile) is never NULL.
 * The file signature key is required job document field in the OTA Agent, so pFileContext->pSignature will
 * never be NULL.
 *
 * If the signature verification fails, file close should still be attempted.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalSignatureCheckFailed: The signature check failed for the specified file.
 *   OtaPalBadSignerCert: The signer certificate was not readable or zero length.
 *   OtaPalFileClose: Error in low level file close.
 */
OtaPalStatus_t otaPal_CloseFile( OtaFileContext_t * const pFileContext )
{
    /* Check the signature. */
    return otaPal_CheckSignature( pFileContext );
}

/**
 * @brief Write a block of data to the specified file at the given offset.
 *
 * @note The input OtaFileContext_t pFileContext is checked for NULL by the OTA agent before this
 * function is called.
 * The file pointer/handle pFileContext->pFile, is checked for NULL by the OTA agent before this
 * function is called.
 * pData is checked for NULL by the OTA agent before this function is called.
 * blockSize is validated for range by the OTA agent before this function is called.
 * offset is validated by the OTA agent before this function is called.
 *
 * @param[in] pFileContext OTA file context information.
 * @param[in] ulOffset Byte offset to write to from the beginning of the file.
 * @param[in] pData Pointer to the byte array of data to write.
 * @param[in] ulBlockSize The number of bytes to write.
 *
 * @return The number of bytes written successfully, or a negative error code from the platform
 * abstraction layer.
 */
int16_t otaPal_WriteBlock( OtaFileContext_t * const pFileContext,
                           uint32_t ulOffset,
                           uint8_t * const pcData,
                           uint32_t ulBlockSize )
{
    uint32_t ulWriteLength, ulDoneLength = 0;

    if( (pFileContext == NULL) || (pFileContext != pxSystemContext ) || ( xOTAComponentID >= FWU_COMPONENT_NUMBER ) )
    {
        return -1;
    }

    while (ulBlockSize > 0)
    {
        ulWriteLength = ulBlockSize <= PSA_FWU_MAX_WRITE_SIZE ?
                        ulBlockSize : PSA_FWU_MAX_WRITE_SIZE;
        /* Call the TF-M Firmware Update service to write image data. */
        if( psa_fwu_write( xOTAComponentID,
                           ( size_t ) ulOffset + ulDoneLength,
                           ( const void * )(pcData + ulDoneLength),
                           ( size_t ) ulWriteLength ) != PSA_SUCCESS )
        {
            return -1;
        }
        ulBlockSize -= ulWriteLength;
        ulDoneLength += ulWriteLength;
    }

    /* If this is the last block, call 'psa_fwu_fnish()' to mark image ready for installation. */
    if( pFileContext->blocksRemaining == 1 )
    {
        LogDebug( ( "pFileContext->blocksRemaining == 1 ." ) );
        if( psa_fwu_finish( xOTAComponentID ) != PSA_SUCCESS )
        {
            return -1;
        }
    }

    return ulDoneLength;
}

/**
 * @brief Activate the newest MCU image received via OTA.
 *
 * This function shall take necessary actions to activate the newest MCU
 * firmware received via OTA. It is typically just a reset of the device.
 *
 * @note This function SHOULD NOT return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalActivateFailed: The activation of the new OTA image failed.
 */
OtaPalStatus_t otaPal_ActivateNewImage( OtaFileContext_t * const pFileContext )
{
    psa_status_t uxStatus;

    if( (pFileContext == NULL) || (pFileContext != pxSystemContext ) || ( xOTAComponentID >= FWU_COMPONENT_NUMBER ) )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalActivateFailed, 0 );
    }

    uxStatus = psa_fwu_install();
    if( (uxStatus == PSA_SUCCESS_REBOOT) || (uxStatus == PSA_SUCCESS_RESTART) )
    {
        otaPal_ResetDevice( pFileContext );

        /* Reset failure happened. */
        return OTA_PAL_COMBINE_ERR( OtaPalActivateFailed, 0 );
    }
    else if( uxStatus == PSA_SUCCESS )
    {
        return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
    }
    else
    {
        return OTA_PAL_COMBINE_ERR( OtaPalActivateFailed, OTA_PAL_SUB_ERR( uxStatus ) );
    }
}

/**
 * @brief Attempt to set the state of the OTA update image.
 *
 * Take required actions on the platform to Accept/Reject the OTA update image (or bundle).
 * Refer to the PAL implementation to determine what happens on your platform.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 * @param[in] eState The desired state of the OTA update image.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 *
 * Major error codes returned are:
 *
 *   OtaPalSuccess on success.
 *   OtaPalBadImageState: if you specify an invalid OtaImageState_t. No sub error code.
 *   OtaPalAbortFailed: failed to roll back the update image as requested by OtaImageStateAborted.
 *   OtaPalRejectFailed: failed to roll back the update image as requested by OtaImageStateRejected.
 *   OtaPalCommitFailed: failed to make the update image permanent as requested by OtaImageStateAccepted.
 */
OtaPalStatus_t otaPal_SetPlatformImageState( OtaFileContext_t * const pFileContext,
                                             OtaImageState_t eState )
{
    psa_fwu_component_t uxComponent;
    psa_status_t uxStatus;
    psa_fwu_component_info_t xComponentInfo = { 0 };

    if( pxSystemContext == NULL )
    {
        /* In this case, a reboot should have happened. */
        switch ( eState )
        {
            case OtaImageStateAccepted:
                if( PortConvertFilePathtoPSAComponentID( pFileContext, &uxComponent ) != OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 ) )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalCommitFailed, 0 );
                }

                /* Make this image as a pernament one. */
                uxStatus = psa_fwu_accept();
                if( uxStatus != PSA_SUCCESS )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalCommitFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                break;
            case OtaImageStateRejected:
                /* If the component is in TRIAL state, the image will be abandoned. Reboot will be carried
                 * out by OTA agent so there is no need to reboot here. */
                uxStatus = psa_fwu_reject( PSA_ERROR_NOT_PERMITTED );
                if(( uxStatus != PSA_SUCCESS ) && ( uxStatus != PSA_SUCCESS_REBOOT ))
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                break;
            case OtaImageStateTesting:
                if( PortConvertFilePathtoPSAComponentID( pFileContext, &uxComponent ) != OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 ) )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalCommitFailed, 0 );
                }
                /* Check if the component is in TRIAL state. */
                uxStatus = psa_fwu_query( uxComponent, &xComponentInfo );
                if( ( uxStatus != PSA_SUCCESS ) || ( xComponentInfo.state != PSA_FWU_TRIAL ) )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalCommitFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                break;
            case OtaImageStateAborted:
                /* The image download has been finished or has not been started.*/
                break;
            default:
                return OTA_PAL_COMBINE_ERR( OtaPalBadImageState, 0 );
        }
    }
    else
    {
        switch ( eState )
        {
            case OtaImageStateAccepted:
                /* The image can only be set as accepted after a reboot. So the pxSystemContext should be NULL. */
                return OTA_PAL_COMBINE_ERR( OtaPalCommitFailed, 0 );
            case OtaImageStateRejected:
                uxStatus = psa_fwu_query( uxComponent, &xComponentInfo );
                if( uxStatus != PSA_SUCCESS )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                if( xComponentInfo.state != PSA_FWU_STAGED )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalBadImageState, 0 );
                }
                uxStatus = psa_fwu_reject( PSA_ERROR_NOT_PERMITTED );
                if(( uxStatus != PSA_SUCCESS ) && ( uxStatus != PSA_SUCCESS_REBOOT ))
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                break;
            case OtaImageStateAborted:
                /* If the component is in TRIAL state, the image will be abandoned. Reboot will be carried
                 * out by OTA agent so there is no need to reboot here. */
                if( PortConvertFilePathtoPSAComponentID( pFileContext, &uxComponent ) != OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 ) )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, 0 );
                }
                uxStatus = psa_fwu_cancel( uxComponent );
                if( uxStatus != PSA_SUCCESS )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, OTA_PAL_SUB_ERR( uxStatus ) );
                }
                if( psa_fwu_clean( xOTAComponentID ) != PSA_SUCCESS )
                {
                    return OTA_PAL_COMBINE_ERR( OtaPalRejectFailed, 0 );
                }
                break;
            default:
                return OTA_PAL_COMBINE_ERR( OtaPalBadImageState, 0 );

        /* The image is still downloading and the OTA process will not continue. The image is in
         * the secondary slot and does not impact the later update process. So nothing to do in
         * other state.
         */
        }
    }
    return OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 );
}

/**
 * @brief Get the state of the OTA update image.
 *
 * We read this at OTA_Init time and when the latest OTA job reports itself in self
 * test. If the update image is in the "pending commit" state, we start a self test
 * timer to assure that we can successfully connect to the OTA services and accept
 * the OTA update image within a reasonable amount of time (user configurable). If
 * we don't satisfy that requirement, we assume there is something wrong with the
 * firmware and automatically reset the device, causing it to roll back to the
 * previously known working code.
 *
 * If the update image state is not in "pending commit," the self test timer is
 * not started.
 *
 * @param[in] pFileContext File context of type OtaFileContext_t.
 *
 * @return An OtaPalImageState_t. One of the following:
 *   OtaPalImageStatePendingCommit (the new firmware image is in the self test phase)
 *   OtaPalImageStateValid         (the new firmware image is already committed)
 *   OtaPalImageStateInvalid       (the new firmware image is invalid or non-existent)
 *
 *   NOTE: OtaPalImageStateUnknown should NEVER be returned and indicates an implementation error.
 */
OtaPalImageState_t otaPal_GetPlatformImageState( OtaFileContext_t * const pFileContext )
{
    psa_status_t uxStatus;
    psa_fwu_component_info_t xComponentInfo = { 0 };
    psa_fwu_component_t uxComponent;

    if( PortConvertFilePathtoPSAComponentID( pFileContext, &uxComponent ) != OTA_PAL_COMBINE_ERR( OtaPalSuccess, 0 ) )
    {
        return OtaPalImageStateInvalid;
    }

    uxStatus = psa_fwu_query( uxComponent, &xComponentInfo );
    if( uxStatus != PSA_SUCCESS )
    {
        return OtaPalImageStateInvalid;
    }
    LogDebug( ( "xComponentInfo.state=%d .", xComponentInfo.state ) );

    switch ( xComponentInfo.state )
    {
        case PSA_FWU_TRIAL:
            return OtaPalImageStatePendingCommit;
        case PSA_FWU_UPDATED:
        case PSA_FWU_READY:
            return OtaPalImageStateValid;
        default:
            return OtaPalImageStateInvalid;
    }

    /* It should never goes here. But just for coding safety. */
    return OtaPalImageStateInvalid;
}
/**
 * @brief Reset the device.
 *
 * This function shall reset the MCU and cause a reboot of the system.
 *
 * @note This function SHOULD NOT return. If it does, the platform does not support
 * an automatic reset or an error occurred.
 *
 * @param[in] pFileContext OTA file context information.
 *
 * @return The OtaPalStatus_t error code is a combination of the main OTA PAL interface error and
 *         the MCU specific sub error code. See ota_platform_interface.h for the OtaPalMainStatus_t
 *         error codes and your specific PAL implementation for the sub error code.
 */
OtaPalStatus_t otaPal_ResetDevice( OtaFileContext_t * const pFileContext )
{
    psa_fwu_request_reboot();
    return OTA_PAL_COMBINE_ERR( OtaPalActivateFailed, 0 );
}
