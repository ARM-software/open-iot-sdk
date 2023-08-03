/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include "Driver_SAI.h"
#include "arm_vsi.h"
#include "device_cfg.h"
#include CMSIS_device_header

#define ARM_SAI_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

/* Audio Peripheral definitions */
#define AudioI         ARM_VSI0_NS      /* Audio Input access struct */
#define AudioI_IRQn    ARM_VSI0_IRQn    /* Audio Input Interrupt number */
#define AudioI_Handler ARM_VSI0_Handler /* Audio Input Interrupt handler */

/* Audio Peripheral registers */
#define CONTROL     Regs[0] /* Control register */
#define CHANNELS    Regs[1] /* Channel register */
#define SAMPLE_BITS Regs[2] /* Sample number of bits (8..32) register */
#define SAMPLE_RATE Regs[3] /* Sample rate (samples per second) register */
#define BLOCK_SIZE  Regs[4] /* DMA block size register */

/* Audio Control register definitions */
#define CONTROL_ENABLE_Pos 0U                          /* CONTROL: ENABLE Position */
#define CONTROL_ENABLE_Msk (1UL << CONTROL_ENABLE_Pos) /* CONTROL: ENABLE Mask */

/* Audio channel control parameter */
#ifndef SAI_NUMBER_OF_CHANNELS
#define SAI_NUMBER_OF_CHANNEL   1U
#endif

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {ARM_SAI_API_VERSION, ARM_SAI_DRV_VERSION};

/* Driver Capabilities */
static const ARM_SAI_CAPABILITIES DriverCapabilities = {
    1, /* supports asynchronous Transmit/Receive */
    0, /* supports synchronous Transmit/Receive */
    1, /* supports user defined Protocol */
    0, /* supports I2S Protocol */
    0, /* supports MSB/LSB justified Protocol */
    0, /* supports PCM short/long frame Protocol */
    0, /* supports AC'97 Protocol */
    0, /* supports Mono mode */
    0, /* supports Companding */
    0, /* supports MCLK (Master Clock) pin */
    0, /* supports Frame error event: \ref ARM_SAI_EVENT_FRAME_ERROR */
    0  /* reserved (must be zero) */
};

/* Event Callback */
static ARM_SAI_SignalEvent_t CB_Event = NULL;

/* Driver State */
static uint8_t Initialized = 0U;

/* Driver Current Power State */
static ARM_POWER_STATE currentPowerState = ARM_POWER_OFF;

/* Driver Current Status */
static ARM_SAI_STATUS currentDriverStatus;

/* VSI helper functions prototypes */
static void ARM_EnableVSIInterrupts(void);
static void ARM_DisableVSIInterrupts(void);

/* Audio Input Interrupt Handler */
void AudioI_Handler(void)
{

    AudioI->IRQ.Clear = 0x00000001U;
    __DSB();
    __ISB();

    currentDriverStatus.rx_busy = 0U;

    if (CB_Event != NULL) {
        CB_Event(ARM_SAI_EVENT_RECEIVE_COMPLETE);
    }
}

//
//  CMSIS Driver APIs
//

static ARM_DRIVER_VERSION ARM_SAI_GetVersion(void)
{
    /* Returns version information of the driver implementation in ARM_DRIVER_VERSION */
    return DriverVersion;
}

static ARM_SAI_CAPABILITIES ARM_SAI_GetCapabilities(void)
{
    /* Retrieves information about the capabilities in this driver implementation. */
    return DriverCapabilities;
}

static int32_t ARM_SAI_Initialize(ARM_SAI_SignalEvent_t cb_event)
{
    CB_Event = cb_event;

    /* Initialize Audio Input Resources */
    AudioI->CONTROL = 0U;
    AudioI->CHANNELS = 0U;
    AudioI->SAMPLE_BITS = 0U;
    AudioI->SAMPLE_RATE = 0U;
    AudioI->BLOCK_SIZE = 0U;

    Initialized = 1U;

    return ARM_DRIVER_OK;
}

static int32_t ARM_SAI_Uninitialize(void)
{
    if (Initialized == 0U) {
        return ARM_DRIVER_ERROR;
    }

    /* Un-initialize Audio Input Resources */
    AudioI->CONTROL = 0U;
    AudioI->CHANNELS = 0U;
    AudioI->SAMPLE_BITS = 0U;
    AudioI->SAMPLE_RATE = 0U;
    AudioI->BLOCK_SIZE = 0U;

    Initialized = 0U;

    return ARM_DRIVER_OK;
}

static int32_t ARM_SAI_PowerControl(ARM_POWER_STATE state)
{
    if (Initialized == 0U) {
        return ARM_DRIVER_ERROR;
    }

    switch (state) {
        case ARM_POWER_OFF:
            /* Disables related interrupts and DMA, disables peripherals, and terminates any pending data transfers */
            ARM_DisableVSIInterrupts();

            AudioI->Timer.Control = 0U;
            AudioI->DMA.Control = 0U;
            AudioI->IRQ.Clear = 0x00000001U;
            AudioI->IRQ.Enable = 0x00000000U;
            currentPowerState = ARM_POWER_OFF;

            /* Transfers cannot be cancelled, instead we return busy status to pause any pending data transfers */
            return ARM_DRIVER_ERROR_BUSY;
        case ARM_POWER_LOW:
            currentPowerState = ARM_POWER_LOW;

            return ARM_DRIVER_ERROR_UNSUPPORTED;
        case ARM_POWER_FULL:
            /* Set-up peripheral for data transfers, enable interrupts (NVIC) and DMA */
            if (currentPowerState != ARM_POWER_FULL) {
                AudioI->DMA.Control = ARM_VSI_DMA_Enable_Msk;
                AudioI->IRQ.Clear = 0x00000001U;
                AudioI->IRQ.Enable = 0x00000001U;

                ARM_EnableVSIInterrupts();

                currentPowerState = ARM_POWER_FULL;
            }
            break;
    }

    return ARM_DRIVER_OK;
}

static int32_t ARM_SAI_Receive(void *data, uint32_t num)
{
    if (Initialized == 0U) {
        return ARM_DRIVER_ERROR;
    }

    /* During the receive operation it is not allowed to call the function again */
    if (currentDriverStatus.rx_busy == 1U) {
        return ARM_DRIVER_ERROR_BUSY;
    }
    currentDriverStatus.rx_busy = 1U;

    if (num > 0) {
        /* The driver shoud configures DMA or the interrupt system for continuous reception */
        AudioI->DMA.Control = 0U;
        AudioI->DMA.Address = (uint32_t)data;
        AudioI->DMA.BlockNum = num;
        AudioI->DMA.BlockSize = AudioI->BLOCK_SIZE;
        uint32_t sample_rate = AudioI->SAMPLE_RATE;
        uint32_t sample_size = (AudioI->CHANNELS * ((AudioI->SAMPLE_BITS + 7U)) / 8U);

        if ((sample_size == 0U) || (sample_rate == 0U)) {
            AudioI->Timer.Interval = 0xFFFFFFFFU;
        }
        else {
            AudioI->Timer.Interval = (1000000U * (AudioI->DMA.BlockSize / sample_size)) / sample_rate;
        }

        AudioI->DMA.Control = ARM_VSI_DMA_Direction_P2M | ARM_VSI_DMA_Enable_Msk;
        AudioI->CONTROL = CONTROL_ENABLE_Msk;
        AudioI->Timer.Control = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Trig_IRQ_Msk | ARM_VSI_Timer_Periodic_Msk
                                | ARM_VSI_Timer_Run_Msk;
    }
    else {
        CB_Event(ARM_SAI_EVENT_RECEIVE_COMPLETE);
    }

    /* The receive function is non-blocking and returns as soon as the driver has started the operation */
    return ARM_DRIVER_OK;
}

static uint32_t ARM_SAI_GetRxCount(void)
{
    if (Initialized == 0U) {
        return ARM_DRIVER_ERROR;
    }

    /* Returns the number of the currently received data items during an ARM_SAI_Receive operation. */
    return (AudioI->Timer.Count);
}

static int32_t ARM_SAI_Control(uint32_t control, uint32_t arg1, uint32_t arg2)
{
    if (Initialized == 0U) {
        return ARM_DRIVER_ERROR;
    }

    /* Enable or disable receiver; arg1 : 0=disable (default); 1=enable */
    if ((control & ARM_SAI_CONTROL_Msk) == ARM_SAI_CONTROL_RX) {
        AudioI->CONTROL = arg1;
    }

    /* Configure transmitter. arg1 and arg2 provide additional configuration options,
     * arg1: DMA Block size
     * arg2: Sampling rate
     */
    if ((control & ARM_SAI_CONTROL_Msk) == ARM_SAI_CONFIGURE_RX) {
        AudioI->CONTROL |= control;
        /* Note: The current implemented protocol is the user defined protocol which serves the Virtual Streaming Interface (VSI) */
        if ((control & ARM_SAI_PROTOCOL_Msk) == ARM_SAI_PROTOCOL_USER) {
            AudioI->CHANNELS = SAI_NUMBER_OF_CHANNEL;
            AudioI->SAMPLE_BITS = ((control & ARM_SAI_DATA_SIZE_Msk) >> ARM_SAI_DATA_SIZE_Pos) + 1U;
            /* The DMA block size and the sampling rates are to be sent as function arguments */
            AudioI->BLOCK_SIZE = arg1;
            AudioI->SAMPLE_RATE = arg2;
        } else {
            /* For other protocols to be implemented */
        }
    }

    return ARM_DRIVER_OK;
}

static ARM_SAI_STATUS ARM_SAI_GetStatus(void)
{
    ARM_SAI_STATUS driverStatus;

    /* rx_busy flag can be copied from the static driver status structure */
    driverStatus.rx_busy = currentDriverStatus.rx_busy;

    /* If the receiver is enabled and data is to be received but the receive operation has not been started yet,
     * then the rx_overflow flag should be set. */
    if ((AudioI->CONTROL & CONTROL_ENABLE_Msk)) {
        if (driverStatus.rx_busy) {
            driverStatus.rx_overflow = 0U;
        }
        else {
            driverStatus.rx_overflow = 1U;
        }
    }
    else {
        driverStatus.rx_overflow = 0U;
    }

    return driverStatus;
}
// End SAI Interface

// VSI helper functions definitions
static void ARM_EnableVSIInterrupts(void)
{
    NVIC_EnableIRQ(ARM_VSI0_IRQn);
}

static void ARM_DisableVSIInterrupts(void)
{
    NVIC_DisableIRQ(ARM_VSI0_IRQn);
}

extern ARM_DRIVER_SAI Driver_SAI0;
ARM_DRIVER_SAI Driver_SAI0 = {ARM_SAI_GetVersion,
                              ARM_SAI_GetCapabilities,
                              ARM_SAI_Initialize,
                              ARM_SAI_Uninitialize,
                              ARM_SAI_PowerControl,
                              NULL,
                              ARM_SAI_Receive,
                              NULL,
                              ARM_SAI_GetRxCount,
                              ARM_SAI_Control,
                              ARM_SAI_GetStatus};
