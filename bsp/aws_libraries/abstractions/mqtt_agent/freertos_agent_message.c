/*
 * FreeRTOS V202104.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
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
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

/**
 * @file freertos_agent_message.c
 * @brief Implements functions to interact with queues.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Header include. */
#include "freertos_agent_message.h"
#include "core_mqtt_agent_message_interface.h"

/*-----------------------------------------------------------*/

bool Agent_MessageSend( MQTTAgentMessageContext_t * pMsgCtx,
                        MQTTAgentCommand_t * const * pCommandToSend,
                        uint32_t blockTimeMs )
{
    osStatus_t queueStatus = osError;

    if( ( pMsgCtx != NULL ) && ( pCommandToSend != NULL ) )
    {
        queueStatus = osMessageQueuePut( pMsgCtx->queue, pCommandToSend, 0U, pdMS_TO_TICKS( blockTimeMs ) );
    }

    return ( queueStatus == osOK ) ? true : false;
}

/*-----------------------------------------------------------*/

bool Agent_MessageReceive( MQTTAgentMessageContext_t * pMsgCtx,
                           MQTTAgentCommand_t ** pReceivedCommand,
                           uint32_t blockTimeMs )
{
    osStatus_t queueStatus = osError;

    if( ( pMsgCtx != NULL ) && ( pReceivedCommand != NULL ) )
    {
        queueStatus = osMessageQueueGet( pMsgCtx->queue, pReceivedCommand, NULL, pdMS_TO_TICKS( blockTimeMs ) );
    }

    return ( queueStatus == osOK ) ? true : false;
}
