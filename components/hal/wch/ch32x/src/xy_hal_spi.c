/**
 * @file xy_hal_spi.c
 * @brief WCH CH32V30x SPI HAL implementation
 */

#include "xy_hal_spi.h"

#ifdef MCU_CH32

#include "ch32v30x.h"

static xy_hal_spi_callback_t spi_callbacks[3];
static void* spi_callback_args[3];

static uint32_t spi_tick_ms(void) {
    return SysTick->CNT / (SystemCoreClock / 1000U);
}

static int spi_index(void* instance) {
    if (instance == SPI1)
        return 0;
    if (instance == SPI2)
        return 1;
    if (instance == SPI3)
        return 2;
    return -1;
}

static void spi_enable_clock(void* instance) {
    if (instance == SPI1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    else if (instance == SPI2)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    else
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
}

static int spi_prescaler_valid(uint32_t value) {
    return value == SPI_BaudRatePrescaler_2 || value == SPI_BaudRatePrescaler_4 ||
           value == SPI_BaudRatePrescaler_8 || value == SPI_BaudRatePrescaler_16 ||
           value == SPI_BaudRatePrescaler_32 || value == SPI_BaudRatePrescaler_64 ||
           value == SPI_BaudRatePrescaler_128 || value == SPI_BaudRatePrescaler_256;
}

static xy_hal_error_t spi_wait(void* instance, uint16_t flag, FlagStatus expected,
                               uint32_t timeout) {
    uint32_t start = spi_tick_ms();
    while (SPI_I2S_GetFlagStatus(instance, flag) != expected) {
        if (timeout != 0U && (spi_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_init(void* instance, const xy_hal_spi_config_t* config) {
    SPI_InitTypeDef init = {0};
    if (spi_index(instance) < 0 || config == NULL || config->mode > XY_HAL_SPI_MODE_3 ||
        config->direction > XY_HAL_SPI_DIR_1LINE || config->datasize > XY_HAL_SPI_DATASIZE_16BIT ||
        config->firstbit > XY_HAL_SPI_FIRSTBIT_LSB || config->nss > XY_HAL_SPI_NSS_HARD_OUTPUT ||
        config->is_master > 1U || !spi_prescaler_valid(config->baudrate_prescaler)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->direction == XY_HAL_SPI_DIR_1LINE || config->nss == XY_HAL_SPI_NSS_HARD_OUTPUT) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    spi_enable_clock(instance);
    init.SPI_Direction = config->direction == XY_HAL_SPI_DIR_2LINES_RXONLY
                             ? SPI_Direction_2Lines_RxOnly
                             : SPI_Direction_2Lines_FullDuplex;
    init.SPI_Mode = config->is_master ? SPI_Mode_Master : SPI_Mode_Slave;
    init.SPI_DataSize =
        config->datasize == XY_HAL_SPI_DATASIZE_16BIT ? SPI_DataSize_16b : SPI_DataSize_8b;
    init.SPI_CPOL = config->mode >= XY_HAL_SPI_MODE_2 ? SPI_CPOL_High : SPI_CPOL_Low;
    init.SPI_CPHA = (config->mode & 1U) != 0U ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    init.SPI_NSS = config->nss == XY_HAL_SPI_NSS_SOFT ? SPI_NSS_Soft : SPI_NSS_Hard;
    init.SPI_BaudRatePrescaler = (uint16_t)config->baudrate_prescaler;
    init.SPI_FirstBit =
        config->firstbit == XY_HAL_SPI_FIRSTBIT_LSB ? SPI_FirstBit_LSB : SPI_FirstBit_MSB;
    init.SPI_CRCPolynomial = 7U;
    SPI_Init(instance, &init);
    if (config->nss == XY_HAL_SPI_NSS_SOFT)
        SPI_NSSInternalSoftwareConfig(instance, SPI_NSSInternalSoft_Set);
    SPI_Cmd(instance, ENABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void* instance) {
    int index = spi_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    SPI_Cmd(instance, DISABLE);
    SPI_I2S_DeInit(instance);
    spi_callbacks[index] = NULL;
    spi_callback_args[index] = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void* instance, const uint8_t* tx_data, uint8_t* rx_data,
                                           size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    if (spi_index(instance) < 0 || (len != 0U && tx_data == NULL && rx_data == NULL)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    for (i = 0U; i < len; ++i) {
        result = spi_wait(instance, SPI_I2S_FLAG_TXE, SET, timeout);
        if (result != XY_HAL_OK)
            return result;
        SPI_I2S_SendData(instance, tx_data == NULL ? 0xFFU : tx_data[i]);
        result = spi_wait(instance, SPI_I2S_FLAG_RXNE, SET, timeout);
        if (result != XY_HAL_OK)
            return result;
        {
            uint8_t received = (uint8_t)SPI_I2S_ReceiveData(instance);
            if (rx_data != NULL)
                rx_data[i] = received;
        }
    }
    return spi_wait(instance, SPI_I2S_FLAG_BSY, RESET, timeout);
}

xy_hal_error_t xy_hal_spi_transmit(void* instance, const uint8_t* data, size_t len,
                                   uint32_t timeout) {
    if (len != 0U && data == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    return xy_hal_spi_transmit_receive(instance, data, NULL, len, timeout);
}

xy_hal_error_t xy_hal_spi_receive(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    if (len != 0U && data == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    return xy_hal_spi_transmit_receive(instance, NULL, data, len, timeout);
}

xy_hal_error_t xy_hal_spi_transmit_dma(void* instance, const uint8_t* data, size_t len) {
    (void)instance;
    (void)data;
    (void)len;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_spi_receive_dma(void* instance, uint8_t* data, size_t len) {
    (void)instance;
    (void)data;
    (void)len;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_spi_transmit_receive_dma(void* instance, const uint8_t* tx_data,
                                               uint8_t* rx_data, size_t len) {
    (void)instance;
    (void)tx_data;
    (void)rx_data;
    (void)len;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_spi_stop(void* instance) {
    return spi_index(instance) < 0 ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_spi_register_callback(void* instance, xy_hal_spi_callback_t callback,
                                            void* arg) {
    int index = spi_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    spi_callbacks[index] = callback;
    spi_callback_args[index] = arg;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_spi_set_cs(void* instance, uint8_t level) {
    (void)level;
    return spi_index(instance) < 0 ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_spi_error(void* instance, uint8_t level) {
    int index = spi_index(instance);
    (void)level;
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (spi_callbacks[index] != NULL) {
        spi_callbacks[index](instance, XY_HAL_SPI_EVENT_ERROR, spi_callback_args[index]);
    }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_spi_set_error_cb(void* instance, xy_hal_spi_callback_t callback, void* arg) {
    return xy_hal_spi_register_callback(instance, callback, arg);
}

#else
#error "WCH SPI backend requires MCU_CH32"
#endif
