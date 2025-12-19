#ifndef _XY_MEM_H_
#define _XY_MEM_H_

#include "xy_typedef.h"

void xy_mem_set(void *p_data, xy_uint8_t set_data, xy_size_t mem_size);
void xy_mem_copy(void *p_dest_data, void *p_src_data, xy_size_t mem_size);
xy_uint8_t xy_mem_cmp(const void *str1, const void *str2, xy_uint8_t len);
void xy_mem_init(void *p, xy_size_t mem_size);
void *xy_mem_malloc(xy_size_t mem_size);
void *xy_mem_malloc_from_irq(xy_size_t mem_size);
void xy_mem_free(void *p);
void xy_mem_free_from_irq(void *p);
xy_uint16_t xy_mem_info(xy_uint16_t *p_num, xy_uint16_t *p_max);

#endif