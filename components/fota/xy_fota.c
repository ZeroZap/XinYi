/**
 * @file xy_fota.c
 * @brief Firmware Over-The-Air Update Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_fota.h"
#include <string.h>

/* FOTA Magic Number */
#define FOTA_MAGIC  0xF0T4A512

static xy_fota_t g_fota;

uint32_t xy_fota_calc_crc32(const uint8_t *data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    
    return ~crc;
}

bool xy_fota_validate_header(const xy_fota_header_t *header)
{
    if (!header) {
        return false;
    }
    
    if (header->magic != FOTA_MAGIC) {
        return false;
    }
    
    if (header->image_size == 0 || header->image_size > XY_FOTA_MAX_IMAGE_SIZE) {
        return false;
    }
    
    return true;
}

int xy_fota_init(xy_fota_t *fota, const xy_fota_config_t *config)
{
    if (!fota || !config) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    memset(fota, 0, sizeof(*fota));
    memcpy(&fota->config, config, sizeof(*config));
    fota->state = XY_FOTA_STATE_IDLE;
    fota->initialized = true;
    
    return XY_FOTA_OK;
}

int xy_fota_deinit(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->initialized = false;
    fota->state = XY_FOTA_STATE_IDLE;
    
    return XY_FOTA_OK;
}

int xy_fota_start_download(xy_fota_t *fota, uint32_t version, uint32_t size)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (size == 0 || size > XY_FOTA_MAX_IMAGE_SIZE) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->header.magic = FOTA_MAGIC;
    fota->header.version = version;
    fota->header.image_size = size;
    fota->header.crc32 = 0;
    fota->downloaded_bytes = 0;
    fota->state = XY_FOTA_STATE_DOWNLOADING;
    
    return XY_FOTA_OK;
}

int xy_fota_download_chunk(xy_fota_t *fota, const uint8_t *data, uint32_t size)
{
    int ret;
    
    if (!fota || !data || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }

    if (fota->state != XY_FOTA_STATE_DOWNLOADING) {
        return XY_FOTA_IN_PROGRESS;
    }

    /* Check if chunk exceeds remaining size */
    if (fota->downloaded_bytes + size > fota->header.image_size) {
        return XY_FOTA_INVALID_PARAM;
    }

    /* Write chunk to flash */
    ret = xy_fota_flash_write(fota, fota->downloaded_bytes, data, size);
    if (ret != XY_FOTA_OK) {
        fota->state = XY_FOTA_STATE_ERROR;
        return XY_FOTA_FLASH_ERROR;
    }

    fota->downloaded_bytes += size;

    /* Call progress callback */
    if (fota->progress_cb) {
        fota->progress_cb(fota->downloaded_bytes, fota->header.image_size, fota->user_data);
    }

    /* Check if download complete */
    if (fota->downloaded_bytes >= fota->header.image_size) {
        fota->state = XY_FOTA_STATE_VALIDATING;
    }

    return XY_FOTA_OK;
}

int xy_fota_finish_download(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (fota->state != XY_FOTA_STATE_VALIDATING) {
        return XY_FOTA_ERROR;
    }
    
    /* Validate downloaded image */
    /* In real implementation, calculate CRC from flash and compare */
    
    fota->state = XY_FOTA_STATE_COMPLETE;
    
    return XY_FOTA_OK;
}

int xy_fota_start_update(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (fota->state != XY_FOTA_STATE_COMPLETE) {
        return XY_FOTA_ERROR;
    }
    
    fota->state = XY_FOTA_STATE_UPDATING;
    
    /* In real implementation:
     * 1. Validate new image
     * 2. Switch boot slot
     * 3. Reset system
     */
    
    fota->state = XY_FOTA_STATE_VERIFYING;
    fota->state = XY_FOTA_STATE_COMPLETE;
    
    return XY_FOTA_OK;
}

xy_fota_state_t xy_fota_get_state(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_STATE_ERROR;
    }
    
    return fota->state;
}

uint8_t xy_fota_get_progress(xy_fota_t *fota)
{
    if (!fota || fota->header.image_size == 0) {
        return 0;
    }
    
    return (fota->downloaded_bytes * 100) / fota->header.image_size;
}

int xy_fota_set_progress_callback(xy_fota_t *fota, xy_fota_progress_cb cb, void *user_data)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->progress_cb = cb;
    fota->user_data = user_data;
    
    return XY_FOTA_OK;
}

int xy_fota_cancel(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->state = XY_FOTA_STATE_IDLE;
    fota->downloaded_bytes = 0;
    
    return XY_FOTA_OK;
}

int xy_fota_reset(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->state = XY_FOTA_STATE_IDLE;
    fota->downloaded_bytes = 0;
    memset(&fota->header, 0, sizeof(fota->header));
    
    return XY_FOTA_OK;
}
