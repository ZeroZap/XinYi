#ifndef _EEEPROM_CFG_H_
#define _EEEPROM_CFG_H_

#if USE_PREF
#stm32, .....choose data width
#endif

#define EEE_DATA_WIDTH_16BITS  0
#define EEE_DATA_WIDTH_32BITS  1
#define EEE_DATA_WIDTH_64BITS  2
#define EEE_DATA_WIDTH_128BITS 3

#define EEE_DATA_WIDTH EEE_DATA_WIDTH_32BITS


#define EEE_SECTOR_SIZE     512
#define EEE_FLASH_ERASE_MAX 20000
#define EEE_FLASH_BASE      0x200000
#define EEE_BLOCK_SIZE      EEE_SECTOR_SIZE
#define EEE_PAGE_SIZE       EEE_BLOCK_SIZE
#define EEE_PAGE_NUM        2
#define EEE_TOTAL_SIZE      (EEE_PAGE_SIZE * EEE_PAGE_NUM)
#define EEE_CYCLE_MAX       (EEE_PAGE_NUM * EEE_FLASH_ERASE_MAX)

// Add flash write granularity configuration
#define EEE_FLASH_WRITE_SIZE 4 // Minimum flash write size in bytes
#define EEE_VERIFY_WRITES    1 // Enable write verification
#define EEE_WRITE_RETRIES    3 // Number of write retries

// Update flash parameters
#define EEE_FLASH_ALIGNMENT (EEE_FLASH_WRITE_SIZE)
#define EEE_DATA_ALIGNMENT  ((EEE_DATA_WIDTH + 7) / 8)
#define EEE_HEADER_SIZE     (EEE_FLASH_WRITE_SIZE)

/**
 * @brief
 * Avg. reliable write/erase cycles of each data
 * Formula：
 * T = (PageSize - PageHeader - DataSize) / DataSize * Page_Cycle * PageNum
 * Page consist of many sectors.
 * PageSize : n*sectorsize <= blocksize
 * PageHeader: It is fixed be to 4 bytes.
 * DataSize: DataNum*4 bytes.
 */


 struct {
    uint16_t address;
    uint16_t value;
 }eee_data_t;
/**
 * @brief
 * extern uint32_t eepram[num];
 *
 */
struct  {
    // if wirte_cnt * 4 == EEE_FLASH_WRITE_SIZE trigger flash write
    uint16_t write_cnt;
    eee_data_t *data;
}eee_db_t
#endif