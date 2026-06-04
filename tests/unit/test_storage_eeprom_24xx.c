#include "xy_eeprom_24xx.h"
#include "xy_hal_error.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define EEPROM_SIZE 512U

typedef struct {
    uint8_t storage[EEPROM_SIZE];
    uint16_t current_addr;
    uint16_t last_dev_addr;
    size_t writes;
    size_t reads;
} fake_i2c_t;

void xy_hal_delay_ms(uint32_t ms)
{
    (void)ms;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle,
                              uint16_t addr, uint32_t timeout)
{
    if (!dev || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    dev->base.initialized = true;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (xy_hal_i2c_master_transmit(dev->i2c_handle, dev->dev_addr, data, len,
                                   dev->timeout) != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }
    return (int)len;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    if (!dev || !data || !dev->base.initialized) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (xy_hal_i2c_master_receive(dev->i2c_handle, dev->dev_addr, data, len,
                                  dev->timeout) != XY_HAL_OK) {
        return XY_DEVICE_IO_ERROR;
    }
    return (int)len;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    fake_i2c_t *fake = (fake_i2c_t *)i2c;
    uint16_t mem_addr;
    size_t offset;
    size_t payload_len;

    (void)timeout;
    assert(fake != NULL);
    assert(data != NULL);
    fake->last_dev_addr = dev_addr;
    fake->writes++;

    if (len == 1U) {
        fake->current_addr = data[0];
        return XY_HAL_OK;
    }

    if (len < 2U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    mem_addr = ((uint16_t)data[0] << 8) | data[1];
    offset = 2U;
    if (mem_addr >= EEPROM_SIZE) {
        mem_addr = data[0];
        offset = 1U;
    }

    payload_len = len - offset;
    if ((size_t)mem_addr + payload_len > EEPROM_SIZE) {
        return XY_HAL_ERROR_OVERFLOW;
    }

    memcpy(&fake->storage[mem_addr], &data[offset], payload_len);
    fake->current_addr = (uint16_t)(mem_addr + payload_len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len,
                                         uint32_t timeout)
{
    fake_i2c_t *fake = (fake_i2c_t *)i2c;

    (void)timeout;
    assert(fake != NULL);
    assert(data != NULL);
    fake->last_dev_addr = dev_addr;
    fake->reads++;

    if ((size_t)fake->current_addr + len > EEPROM_SIZE) {
        return XY_HAL_ERROR_OVERFLOW;
    }

    memcpy(data, &fake->storage[fake->current_addr], len);
    fake->current_addr = (uint16_t)(fake->current_addr + len);
    return XY_HAL_OK;
}

static void test_init_and_argument_validation(void)
{
    fake_i2c_t fake;
    xy_eeprom_24xx_t eeprom;

    memset(&fake, 0, sizeof(fake));
    memset(fake.storage, 0xFF, sizeof(fake.storage));
    assert(xy_eeprom_24xx_init(NULL, &fake, 0x50, 16, EEPROM_SIZE) ==
           XY_DEVICE_INVALID_PARAM);
    assert(xy_eeprom_24xx_init(&eeprom, NULL, 0x50, 16, EEPROM_SIZE) ==
           XY_DEVICE_INVALID_PARAM);
    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x50, 0, EEPROM_SIZE) ==
           XY_DEVICE_INVALID_PARAM);
    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x50, 16, 0) ==
           XY_DEVICE_INVALID_PARAM);

    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x50, 16, EEPROM_SIZE) ==
           XY_DEVICE_OK);
    assert(eeprom.page_size == 16U);
    assert(eeprom.total_size == EEPROM_SIZE);
    assert(eeprom.address_bits == 16U);
    assert(eeprom.i2c_dev.i2c_handle == &fake);
    assert(eeprom.i2c_dev.dev_addr == 0x50U);
}

static void test_write_read_and_page_splitting(void)
{
    fake_i2c_t fake;
    xy_eeprom_24xx_t eeprom;
    uint8_t payload[20];
    uint8_t out[20];

    memset(&fake, 0, sizeof(fake));
    memset(fake.storage, 0xFF, sizeof(fake.storage));
    memset(out, 0, sizeof(out));
    for (size_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)(0xA0U + i);
    }

    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x50, 16, EEPROM_SIZE) ==
           XY_DEVICE_OK);
    assert(xy_eeprom_24xx_write(&eeprom, 14, payload, sizeof(payload)) ==
           (int)sizeof(payload));
    assert(fake.writes == 3U);
    assert(memcmp(&fake.storage[14], payload, sizeof(payload)) == 0);

    assert(xy_eeprom_24xx_read(&eeprom, 14, out, sizeof(out)) == (int)sizeof(out));
    assert(fake.last_dev_addr == 0x50U);
    assert(fake.reads == 1U);
    assert(memcmp(out, payload, sizeof(out)) == 0);
}

static void test_8bit_address_devices(void)
{
    fake_i2c_t fake;
    xy_eeprom_24xx_t eeprom;
    const uint8_t payload[] = {1, 2, 3, 4};
    uint8_t out[sizeof(payload)] = {0};

    memset(&fake, 0, sizeof(fake));
    memset(fake.storage, 0xFF, sizeof(fake.storage));
    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x51, 8, 128) == XY_DEVICE_OK);
    assert(eeprom.address_bits == 8U);
    assert(xy_eeprom_24xx_write(&eeprom, 6, payload, sizeof(payload)) ==
           (int)sizeof(payload));
    assert(memcmp(&fake.storage[6], payload, sizeof(payload)) == 0);
    assert(xy_eeprom_24xx_read(&eeprom, 6, out, sizeof(out)) == (int)sizeof(out));
    assert(memcmp(out, payload, sizeof(out)) == 0);
}

static void test_bounds_and_page_write_contracts(void)
{
    fake_i2c_t fake;
    xy_eeprom_24xx_t eeprom;
    uint8_t payload[8] = {0};

    memset(&fake, 0, sizeof(fake));
    memset(fake.storage, 0xFF, sizeof(fake.storage));
    assert(xy_eeprom_24xx_init(&eeprom, &fake, 0x50, 8, 32) == XY_DEVICE_OK);
    assert(xy_eeprom_24xx_read(&eeprom, 31, payload, 2) == XY_DEVICE_INVALID_PARAM);
    assert(xy_eeprom_24xx_write(&eeprom, 31, payload, 2) == XY_DEVICE_INVALID_PARAM);
    assert(xy_eeprom_24xx_write_page(&eeprom, 6, payload, sizeof(payload)) == 2);
}

int main(void)
{
    test_init_and_argument_validation();
    test_write_read_and_page_splitting();
    test_8bit_address_devices();
    test_bounds_and_page_write_contracts();
    return 0;
}
