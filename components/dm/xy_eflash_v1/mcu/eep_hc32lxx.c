#include "eep_port.h"
#include "flash.h"

int32_t eep_flash_write_words(uint32_t addr, const uint32_t *data, uint16_t len)
{
    // if write len <= 0 return eep_error_t
    int32_t ret = 0;
    ///< STEP-4 FLASH 字节写、校验
    while (Ok != Flash_OpModeConfig(FlashWriteMode)) {
        ///< 用户可以根据自身使用需求添加超时处理代码
    }

    if (Ok == Flash_Write32(addr, data, len)) {
        ret = len;
    }
    return ret;
}

int32_t eep_flash_read_words(uint32_t addr, uint32_t *data, uint16_t len)
{
    // if write len <= 0 return eep_error_t
    int32_t ret = 0;
    int16_t i   = 0;
    for (i = 0; i < len; i++) {
        data[i] = *((volatile uint32_t *)addr);
        addr += 4;
    }
    return ret;
}

int32_t eep_flash_erase(uint32_t addr, uint32_t len)
{
    // if write len <= 0 return eep_error_t
    int32_t ret = 0;
    while (Ok != Flash_OpModeConfig(FlashSectorEraseMode)) {
        ///< 用户可以根据自身使用需求添加超时处理代码
    }
    if (Ok != Flash_SectorErase(addr)) {
        while (1) {
            ///< 用户可以根据自身使用需求添加超时处理代码
        }
    }
    return ret;
}

int32_t eep_flash_init(void)
{
    ///< STEP-1:
    ///< 确保初始化正确执行后方能进行FLASH编程操作，FLASH初始化（编程时间,休眠模式配置）
    while (Ok != Flash_Init(1, TRUE)) {
        ///< 用户可以根据自身使用需求添加超时处理代码
    }

    ///< STEP-2 FLASH 操作区解锁(Sector252~255)
    while (Ok != Flash_LockSet(FlashLock1, 0x80000000)) {
        ///< 用户可以根据自身使用需求添加超时处理代码
    }
		
		return 0;
}
