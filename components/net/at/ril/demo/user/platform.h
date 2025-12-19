/******************************************************************************
* @file     platform.h
* @brief    
* @version  1.0
* @date     2019-03-25
* @author   roger.luo
* 
* Coypright (C) 2018 
* All rights reserved.
*
*******************************************************************************/
#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdbool.h>

#pragma section = "HEAP"
#define HEAP_BEGIN  (__section_begin("HEAP"))
#define HEAP_END    (__section_end("HEAP"))

#endif