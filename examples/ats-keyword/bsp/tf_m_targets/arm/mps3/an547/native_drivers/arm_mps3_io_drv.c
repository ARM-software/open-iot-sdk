/*
 * Copyright (c) 2018 ARM Limited
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

#include "arm_mps3_io_drv.h"

/* There is at most 10 LEDs 8 switches and 2 buttons on MPS3 FPGA IO */
#define MAX_PIN_FPGAIO_LED         10UL
#define MAX_PIN_FPGAIO_BUTTON      2UL
#define MAX_PIN_FPGAIO_SWITCH      8UL

/* Mask to 1 the first X bits */
#define MASK(X)            ((1UL << (X)) - 1)

/* MPS3 IO register map structure */
struct arm_mps3_io_reg_map_t {
    volatile uint32_t fpgaio_leds;      /* Offset: 0x000 (R/W) LED connections
                                         *         [31:10] : Reserved
                                         *         [9:0]  : FPGAIO LEDs */
    volatile uint32_t reserved[1];
    volatile uint32_t fpgaio_buttons;   /* Offset: 0x008 (R/ ) Buttons
                                         *         [31:2] : Reserved
                                         *         [1:0]  : Buttons */
    volatile uint32_t reserved2[7];
    volatile uint32_t fpgaio_switches;  /* Offset: 0x028 (R/ ) Denotes the
                                         *                  state of the FPGAIO
                                         *                  user switches
                                         *         [31:8] : Reserved
                                         *         [7:0]  : FPGAIO switches */
};

void arm_mps3_io_write_leds(struct arm_mps3_io_dev_t* dev,
                            enum arm_mps3_io_access_t access,
                            uint8_t pin_num,
                            uint32_t value)
{
    struct arm_mps3_io_reg_map_t* p_mps3_io_port =
                                  (struct arm_mps3_io_reg_map_t*)dev->cfg->base;
    /* Mask of involved bits */
    uint32_t write_mask = 0;

    if (pin_num >= MAX_PIN_FPGAIO_LED) {
        return;
    }

    switch (access) {
    case ARM_MPS3_IO_ACCESS_PIN:
        write_mask = (1UL << pin_num);
        break;
    case ARM_MPS3_IO_ACCESS_PORT:
        write_mask = MASK(MAX_PIN_FPGAIO_LED);
        break;
    /*
     * default: explicitely not used to force to cover all enumeration
     * cases
     */
    }

    if (value) {
        p_mps3_io_port->fpgaio_leds |= write_mask;
    } else {
        p_mps3_io_port->fpgaio_leds &= ~write_mask;
    }

}

uint32_t arm_mps3_io_read_buttons(struct arm_mps3_io_dev_t* dev,
                                  enum arm_mps3_io_access_t access,
                                  uint8_t pin_num)
{
    struct arm_mps3_io_reg_map_t* p_mps3_io_port =
                                  (struct arm_mps3_io_reg_map_t*)dev->cfg->base;
    uint32_t value = 0;

    if (pin_num >= MAX_PIN_FPGAIO_BUTTON) {
        return 0;
    }

    /* Only read significant bits from this register */
    value = p_mps3_io_port->fpgaio_buttons &
            MASK(MAX_PIN_FPGAIO_BUTTON);

    if (access == ARM_MPS3_IO_ACCESS_PIN) {
        value = ((value >> pin_num) & 1UL);
    }

    return value;
}

uint32_t arm_mps3_io_read_switches(struct arm_mps3_io_dev_t* dev,
                                  enum arm_mps3_io_access_t access,
                                  uint8_t pin_num)
{
    struct arm_mps3_io_reg_map_t* p_mps3_io_port =
                                  (struct arm_mps3_io_reg_map_t*)dev->cfg->base;
    uint32_t value = 0;

    if (pin_num >= MAX_PIN_FPGAIO_SWITCH) {
        return 0;
    }

    /* Only read significant bits from this register */
    value = p_mps3_io_port->fpgaio_switches &
            MASK(MAX_PIN_FPGAIO_SWITCH);


    if (access == ARM_MPS3_IO_ACCESS_PIN) {
        value = ((value >> pin_num) & 1UL);
    }

    return value;
}

uint32_t arm_mps3_io_read_leds(struct arm_mps3_io_dev_t* dev,
                               enum arm_mps3_io_access_t access,
                               uint8_t pin_num)
{
    struct arm_mps3_io_reg_map_t* p_mps3_io_port =
                                  (struct arm_mps3_io_reg_map_t*)dev->cfg->base;
    uint32_t value = 0;

    if (pin_num >= MAX_PIN_FPGAIO_LED) {
        return 0;
    }

    /* Only read significant bits from this register */
    value = p_mps3_io_port->fpgaio_leds & MASK(MAX_PIN_FPGAIO_LED);

    if (access == ARM_MPS3_IO_ACCESS_PIN) {
        value = ((value >> pin_num) & 1UL);
    }

    return value;
}
