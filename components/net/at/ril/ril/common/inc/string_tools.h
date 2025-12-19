/******************************************************************************
 * @brief        字符串处理工具
 *
 * Copyright (c) 2016~2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2016-05-10     Morro.luo    Initial version.
 * 2019-03-05     Morro.luo    Add memrstr.
 ******************************************************************************/
#ifndef _STRING_TOOLS_H_
#define _STRING_TOOLS_H_

char *strupr(char *s);
char *strlwr(char *s);
int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, int n);
int strsplit(char *s, const char *separator, char *list[],  int len);
int strmerge(int argc, char *argv[], int start, int end, char connector);
char *strtrim(char *s, const char *trim_chars);
const char *strskip(const char *s, const char *skip_chars);
char *memstr(void *mem, int len, char *str);
char *memrstr(void *mem, int len, char *str);


#endif

