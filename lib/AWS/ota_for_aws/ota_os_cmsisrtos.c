/*
 * AWS IoT Over-the-air Update v3.1.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 */

/**
 * @file ota_os_cmsisrtos.c
 */

/* OTA OS POSIX Interface Includes.*/
#include "ota_os_cmsisrtos.h"

/* OTA Library include. */
#include "ota.h"
#include "ota_private.h"

#include "cmsis_os2.h"
#include "RTE_Components.h"

#include <stdlib.h>

/* OTA Event queue attributes.*/
#define MAX_MESSAGES 20
#define MAX_MSG_SIZE sizeof(OtaEventMsg_t)

/* The queue control handle.  .*/
static osMessageQueueId_t otaEventQueue;

/* OTA App Timer callback.*/
static OtaTimerCallback_t otaTimerCallback;

/* OTA Timer handles.*/
static osTimerId_t otaTimer[OtaNumOfTimers];

/* OTA Timer callbacks.*/
static void requestTimerCallback(osTimerId_t T);
static void selfTestTimerCallback(osTimerId_t T);
void (*timerCallback[OtaNumOfTimers])(osTimerId_t T) = {requestTimerCallback, selfTestTimerCallback};

OtaOsStatus_t OtaInitEvent_CMSISRTOS(OtaEventContext_t *pEventCtx)
{
    (void)pEventCtx;

    otaEventQueue = osMessageQueueNew(MAX_MESSAGES, MAX_MSG_SIZE, NULL);

    if (otaEventQueue == NULL) {
        LogError(("Failed to create OTA Event Queue: "
                  "xQueueCreateStatic returned error: "
                  "OtaOsStatus_t=%i ",
                  OtaOsEventQueueCreateFailed));
        return OtaOsEventQueueCreateFailed;
    } else {
        LogDebug(("OTA Event Queue created."));
    }

    return OtaOsSuccess;
}

OtaOsStatus_t OtaSendEvent_CMSISRTOS(OtaEventContext_t *pEventCtx, const void *pEventMsg, unsigned int timeout)
{
    (void)pEventCtx;
    (void)timeout;

    /* Send the event to OTA event queue.*/
    osStatus_t retVal = osMessageQueuePut(otaEventQueue, pEventMsg, 0, 0);

    if (retVal == osOK) {
        LogDebug(("OTA Event Sent."));
    } else {
        LogError(("Failed to send event to OTA Event Queue: "
                  "osMessageQueuePut returned error: "
                  "OtaOsStatus_t=%i ",
                  OtaOsEventQueueSendFailed));
        return OtaOsEventQueueSendFailed;
    }

    return OtaOsSuccess;
}

OtaOsStatus_t OtaReceiveEvent_CMSISRTOS(OtaEventContext_t *pEventCtx, void *pEventMsg, uint32_t timeout)
{
    /* Temp buffer.*/
    uint8_t buff[sizeof(OtaEventMsg_t)];

    (void)pEventCtx;
    (void)timeout;

    osStatus_t retVal = osMessageQueueGet(otaEventQueue, &buff, 0, osWaitForever);

    if (retVal == osOK) {
        /* copy the data from local buffer.*/
        memcpy(pEventMsg, buff, MAX_MSG_SIZE);
        LogDebug(("OTA Event received"));
    } else {
        LogError(("Failed to receive event from OTA Event Queue: "
                  "xQueueReceive returned error: "
                  "OtaOsStatus_t=%i ",
                  OtaOsEventQueueReceiveFailed));
        return OtaOsEventQueueSendFailed;
    }

    return OtaOsSuccess;
}

OtaOsStatus_t OtaDeinitEvent_CMSISRTOS(OtaEventContext_t *pEventCtx)
{
    (void)pEventCtx;

    /* Remove the event queue.*/
    if (otaEventQueue != NULL) {
        osMessageQueueDelete(otaEventQueue);
        otaEventQueue = NULL;

        LogDebug(("OTA Event Queue Deleted."));
    }

    return OtaOsSuccess;
}

static void selfTestTimerCallback(osTimerId_t T)
{
    (void)T;

    LogDebug(("Self-test expired within %ums\r\n", otaconfigSELF_TEST_RESPONSE_WAIT_MS));

    if (otaTimerCallback != NULL) {
        otaTimerCallback(OtaSelfTestTimer);
    } else {
        LogWarn(("Self-test timer event unhandled.\r\n"));
    }
}

static void requestTimerCallback(osTimerId_t T)
{
    (void)T;

    LogDebug(("Request timer expired in %ums \r\n", otaconfigFILE_REQUEST_WAIT_MS));

    if (otaTimerCallback != NULL) {
        otaTimerCallback(OtaRequestTimer);
    } else {
        LogWarn(("Request timer event unhandled.\r\n"));
    }
}

OtaOsStatus_t OtaStartTimer_CMSISRTOS(OtaTimerId_t otaTimerId,
                                      const char *const pTimerName,
                                      const uint32_t timeout,
                                      OtaTimerCallback_t callback)
{
    OtaOsStatus_t otaOsStatus = OtaOsSuccess;

    configASSERT(callback != NULL);
    configASSERT(pTimerName != NULL);

    /* Set OTA lib callback. */
    otaTimerCallback = callback;

    /* If timer is not created.*/
    if (otaTimer[otaTimerId] == NULL) {
        /* Create the timer. */
        osTimerAttr_t attr = {
            .name = pTimerName,
        };
        otaTimer[otaTimerId] = osTimerNew(timerCallback[otaTimerId], osTimerOnce, NULL, &attr);

        if (otaTimer[otaTimerId] == NULL) {
            otaOsStatus = OtaOsTimerCreateFailed;

            LogError(("Failed to create OTA timer: "
                      "timerCreate returned NULL "
                      "OtaOsStatus_t=%i ",
                      otaOsStatus));
        } else {
            LogDebug(("OTA Timer created."));

            /* Start the timer. */
            osStatus_t retVal = osTimerStart(otaTimer[otaTimerId], pdMS_TO_TICKS(timeout));

            if (retVal == osOK) {
                LogDebug(("OTA Timer started."));
            } else {
                otaOsStatus = OtaOsTimerStartFailed;

                LogError(("Failed to start OTA timer: "
                          "timerStart returned error."));
            }
        }
    } else {
        /* Reset the timer. */
        osStatus_t retVal = osTimerStart(otaTimer[otaTimerId], pdMS_TO_TICKS(timeout));

        if (retVal == osOK) {
            LogDebug(("OTA Timer restarted."));
        } else {
            otaOsStatus = OtaOsTimerRestartFailed;

            LogError(("Failed to set OTA timer timeout: "
                      "timer_settime returned error: "
                      "OtaOsStatus_t=%i ",
                      otaOsStatus));
        }
    }

    return otaOsStatus;
}

OtaOsStatus_t OtaStopTimer_CMSISRTOS(OtaTimerId_t otaTimerId)
{
    if (otaTimer[otaTimerId] != NULL) {
        /* Stop the timer. */
        osStatus_t retVal = osTimerStop(otaTimer[otaTimerId]);

        if (retVal == osOK) {
            LogDebug(("OTA Timer Stopped for Timerid=%i.", otaTimerId));
        } else {
            LogError(("Failed to stop OTA timer: "
                      "timer_settime returned error: "
                      "OtaOsStatus_t=%i ",
                      OtaOsTimerStopFailed));

            return OtaOsTimerStopFailed;
        }
    } else {
        LogWarn(("OTA Timer handle NULL for Timerid=%i, can't stop.", otaTimerId));
    }

    return OtaOsSuccess;
}

OtaOsStatus_t OtaDeleteTimer_CMSISRTOS(OtaTimerId_t otaTimerId)
{
    OtaOsStatus_t otaOsStatus = OtaOsSuccess;

    if (otaTimer[otaTimerId] != NULL) {
        /* Delete the timer. */
        osStatus_t retVal = osTimerDelete(otaTimer[otaTimerId]);

        if (retVal == osOK) {
            otaTimer[otaTimerId] = NULL;
            LogDebug(("OTA Timer deleted."));
        } else {
            otaOsStatus = OtaOsTimerDeleteFailed;

            LogError(("Failed to delete OTA timer: "
                      "timer_delete returned error: "
                      "OtaOsStatus_t=%i ",
                      otaOsStatus));
        }
    } else {
        otaOsStatus = OtaOsTimerDeleteFailed;

        LogWarn(("OTA Timer handle NULL for Timerid=%i, can't delete.", otaTimerId));
    }

    return otaOsStatus;
}

void *Malloc_CMSISRTOS(size_t size)
{
    return pvPortMalloc(size);
}

void Free_CMSISRTOS(void *ptr)
{
    vPortFree(ptr);
}
