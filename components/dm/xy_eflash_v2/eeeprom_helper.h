#ifndef _EEEPROM_HELPER_H_
#define _EEEPROM_HELPER_H_


#if ((EEE_BLOCK_SIZE - EEE_ADDR_SIZE) < EEE_USER_DATA_SIZE)
#error "USER data too large..."
#endif


// 针对不定长数据
// uint16_t 可以做拆分
// 高 2 位 做类型
// 中 2 位 做校验值
// 低 12 位 做地址 // 4k的地址，一般都是 4k的了
// 支持 16 bit， 32 bit，32bit的数字
#endif