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

/**
 * \file arm_mps3_io_drv.h
 * \brief Generic driver for ARM MPS3 FPGAIO.
 */

#ifndef __ARM_MPS3_IO_DRV_H__
#define __ARM_MPS3_IO_DRV_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARM MPS3 IO enumeration types */
enum arm_mps3_io_access_t {
    ARM_MPS3_IO_ACCESS_PIN = 0,  /*!< Pin access to MPS3 IO */
    ARM_MPS3_IO_ACCESS_PORT      /*!< Port access to MPS3 IO */
};

/* ARM MPS3 IO device configuration structure */
struct arm_mps3_io_dev_cfg_t {
    const uint32_t base;                 /*!< MPS3 IO base address */
};

/* ARM MPS3 IO device structure */
struct arm_mps3_io_dev_t {
    const struct arm_mps3_io_dev_cfg_t* const cfg; /*!< MPS3 IO configuration */
};

/**
 * \brief  Writes to output LEDs.
 *
 * \param[in] dev      MPS3 IO device where to write \ref arm_mps3_io_dev_t
 * \param[in] access   Access type \ref arm_mps3_io_access_t
 * \param[in] pin_num  Pin number.
 * \param[in] value    Value(s) to set.
 *
 * \note This function doesn't check if dev is NULL.
 */
void arm_mps3_io_write_leds(struct arm_mps3_io_dev_t* dev,
                            enum arm_mps3_io_access_t access,
                            uint8_t pin_num,
                            uint32_t value);

/**
 * \brief Reads the buttons status.
 *
 * \param[in] dev      MPS3 IO device where to read \ref arm_mps3_io_dev_t
 * \param[in] access   Access type \ref arm_mps3_io_access_t
 * \param[in] pin_num  Pin number.
 *
 * \return Returns bit value for Pin access or port value for port access.
 *
 * \note This function doesn't check if dev is NULL.
 */
uint32_t arm_mps3_io_read_buttons(struct arm_mps3_io_dev_t* dev,
                                  enum arm_mps3_io_access_t access,
                                  uint8_t pin_num);

/**
 * \brief Reads the switches status.
 *
 * \param[in] dev      MPS3 IO device where to read \ref arm_mps3_io_dev_t
 * \param[in] access   Access type \ref arm_mps3_io_access_t
 * \param[in] pin_num  Pin number.
 *
 * \return Returns bit value for Pin access or port value for port access.
 *
 * \note This function doesn't check if dev is NULL.
 */
 uint32_t arm_mps3_io_read_switches(struct arm_mps3_io_dev_t* dev,
                                      enum arm_mps3_io_access_t access,
                                      uint8_t pin_num);

/**
 * \brief Reads the LED status.
 *
 * \param[in] dev      MPS3 IO device where to read \ref arm_mps3_io_dev_t
 * \param[in] access   Access type \ref arm_mps3_io_access_t
 * \param[in] pin_num  Pin number.
 *
 * \return Returns bit value for Pin access or port value for port access.
 *
 * \note This function doesn't check if dev is NULL.
 */
uint32_t arm_mps3_io_read_leds(struct arm_mps3_io_dev_t* dev,
                               enum arm_mps3_io_access_t access,
                               uint8_t pin_num);

#ifdef __cplusplus
}
#endif

#endif /* __ARM_MPS3_IO_DRV_H__ */
