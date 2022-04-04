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
#include "FreeRTOS.h"
#include "task.h"
#include "tfm_ns_interface.h"
#include "psa/protected_storage.h"
#include "update_task.h"
#include "flash_layout.h"
#include "bootutil/image.h"
#include "string.h"
#include "hal_config.h"
#include "cmsis.h"
#include "tfm_ioctl_api.h"
#include "tfm_fwu_defs.h"
#include "psa/update.h"

#define UPDATE_SECURE_IMAGE_ADDRESS              (uint8_t *)0x62000000
#define UPDATE_SECURE_IMAGE_ADDRESS_SECURE_ALIAS (uint8_t *)0x62000000
#define UPDATE_NONSECURE_IMAGE_ADDRESS           (uint8_t *)0x62060000
#define SECONDARY_SLOT_S_ADDRESS                 (uint8_t *)(QSPI_SRAM_BASE_S + FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE)
#define SECONDARY_SLOT_NS_ADDRESS \
    (uint8_t *)(QSPI_SRAM_BASE_NS + (2 * FLASH_S_PARTITION_SIZE) + FLASH_NS_PARTITION_SIZE)

typedef enum update_type_t {
    UPDATE_TYPE_SECURE = 0x1,
    UPDATE_TYPE_NONSECURE = 0x2,
    UPDATE_TYPE_BOTH = 0x3
} update_type_t;

static uint32_t verify_update(uint8_t *update_address)
{
    struct image_header *hdr = (struct image_header *)update_address;

    if (hdr->ih_magic == IMAGE_MAGIC) {
        uint32_t total_size = hdr->ih_hdr_size + hdr->ih_img_size;
        info("Found update payload of size %d\n", total_size);
        return total_size;
    }

    return 0;
}

static void invalidate_update_slot(bool secure)
{
    const psa_image_id_t update_img_id = (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(
        FWU_IMAGE_ID_SLOT_STAGE, secure ? FWU_IMAGE_TYPE_SECURE : FWU_IMAGE_TYPE_NONSECURE, 0 /* id */
    );

    psa_fwu_abort(update_img_id);
}

static bool copy_update_to_slot(
    uint8_t *update_address, uint8_t *slot_address, uint32_t update_size, uint32_t slot_size, bool secure)
{
    if (update_size > slot_size) {
        warn("Update does not fit in slot\n");
        return false;
    }

    if (secure) {
        tfm_pin_service_args_t args = {slot_address, update_address, update_size};
        psa_invec input = {.base = &args, .len = sizeof(tfm_pin_service_args_t)};
        enum tfm_platform_err_t ret = tfm_platform_ioctl(TFM_PLATFORM_IOCTL_MEMCPY_SERVICE, &input, NULL);
        if (ret != TFM_PLATFORM_ERR_SUCCESS) {
            return false;
        }
    } else {
        const psa_image_id_t update_img_id =
            (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE, FWU_IMAGE_TYPE_NONSECURE, 0 /* id */
            );

        psa_image_info_t info;
        psa_image_id_t dependency_uuid;
        psa_image_version_t dependency_version;
        psa_status_t status;

        /* this writes the whole image despite being larger than PSA_FWU_MAX_BLOCK_SIZE */
        status = psa_fwu_write(update_img_id, 0, update_address, slot_size);
        if (status != PSA_SUCCESS) {
            warn("psa_fwu_write failed\n");
            return false;
        }

        status = psa_fwu_query(update_img_id, &info);
        if (status != PSA_SUCCESS) {
            warn("psa_fwu_query failed\n");
            return false;
        }

        if (info.state != PSA_IMAGE_CANDIDATE) {
            warn("Image should be in PSA_IMAGE_CANDIDATE state after successful write\n");
            return false;
        }

        status = psa_fwu_install(update_img_id, &dependency_uuid, &dependency_version);
        if (status != PSA_SUCCESS_REBOOT) {
            warn("psa_fwu_install failed\n");
            return false;
        }
    }

    info("Update copied to %x\n", (unsigned)slot_address);

    /* invalidate payload */
    ((struct image_header *)update_address)->ih_magic = 0;
    return true;
}

static update_type_t check_for_update()
{
    /* TODO: make call to aws client to check update and download payload */
    /* this could be blocking, or not, delay for now */
    vTaskDelay(1);

    return UPDATE_TYPE_NONSECURE;
}

static void perform_update()
{
    info("calling system reset\n");
    /* would be best to delay to convenient moment currently reset immediately */
    psa_fwu_request_reboot();
}

void update_task(void *pvParameters)
{
    while (1) {
        uint32_t ns_size = 0;
        uint32_t s_size = 0;

        info("checking for update\n");

        update_type_t update_type = check_for_update();

        if (update_type & UPDATE_TYPE_NONSECURE) {
            ns_size = verify_update(UPDATE_NONSECURE_IMAGE_ADDRESS);
            if (!ns_size) {
                warn("Nonsecure update corrupted, aborting\n");
                continue;
            }
        }

        if (update_type & UPDATE_TYPE_SECURE) {
            s_size = verify_update(UPDATE_SECURE_IMAGE_ADDRESS);
            if (!s_size) {
                warn("Secure update corrupted, aborting\n");
                continue;
            }
        }

        if (ns_size) {
            if (!copy_update_to_slot(UPDATE_NONSECURE_IMAGE_ADDRESS,
                                     SECONDARY_SLOT_NS_ADDRESS,
                                     ns_size,
                                     FLASH_NS_PARTITION_SIZE,
                                     false)) {
                warn("Nonsecure Update copy failed, aborting\n");
                continue;
            }
        }

        if (s_size) {
            if (!copy_update_to_slot(UPDATE_SECURE_IMAGE_ADDRESS_SECURE_ALIAS,
                                     SECONDARY_SLOT_S_ADDRESS,
                                     s_size,
                                     FLASH_S_PARTITION_SIZE,
                                     true)) {
                warn("Secure update copy failed, aborting and cancelling nonsecure update\n");
                invalidate_update_slot(false);
                continue;
            }
        }

        perform_update();
    }
}
