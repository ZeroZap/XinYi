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
#include "string_tools.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>


/**
 * @brief       将字符串s中的小写字符转换成大写(其它不转换)
 * @param[in]   s - 待转换的字符串
 * @return      返回转换后的字符串的指针
 */  
char *strupr(char *s)
{
    while (*s != '\0') {
        *s = toupper(*s);
        s++;  
    }
    return s;
}

/**
 * @brief       将字符串s中的大写字符转换成小写(其它不转换)
 * @param[in]   s - 待转换的字符串
 * @return      返回转换后的字符串的指针
 */  
char *strlwr(char *s)
{
    while (*s != '\0') {
        *s = tolower(*s);
        s++;  
    }
    return s;
}

/**
 * @brief       字符串比较,不区分大小写
 * @param[in]   s1,s2 - 需要比较的字符串指针
 * @return      <0 if s1<s2, 0 if s1==s2, or >0 if s1>s2.
 */ 
int stricmp(const char *s1, const char *s2)
{
    char c1, c2;  
    c1 = 0; c2 = 0;  

        do 
        {  
            c1 = *s1; c2 = *s2;  
            s1++; s2++;          
            if (!c1)break;  
            if (!c2)break;               
            if (c1 == c2)  
                continue;            
            c1 = tolower(c1);  
            c2 = tolower(c2);   
            if (c1 != c2)  
                break;  
        } while (*s1 && *s2);  
    
    if(*s1 == 0&&  *s2 == 0)
        return (int)c1 - (int)c2;
    else
        return tolower(c1) - tolower(c2);
}

/**
 * @brief       带长度字符串比较,不区分大小写
 * @param[in]   s1,s2 - 需要比较的字符串指针
 * @param[in]   n - 比较的字符串长度
 * @return      <0 if s1<s2, 0 if s1==s2, or >0 if s1>s2.
 */  
int strnicmp(const char *s1, const char *s2, int n)
{
    char c1, c2;  
    c1 = 0; c2 = 0;  
    if (n) 
    {  
        do 
        {  
            c1 = *s1; c2 = *s2;  
            s1++; s2++;          
            if (!c1)break;  
            if (!c2)break;               
            if (c1 == c2)  
                continue;            
            c1 = tolower(c1);  
            c2 = tolower(c2);   
            if (c1 != c2)  
                break;  
        } while (--n);  
    }  
    return (int)c1 - (int)c2;  
}

/**
 * @brief      字符串分割  - 在源字符串查找出所有由separator指定的分隔符
 *                            (如',')并替换成字符串结束符'\0'形成子串，同时令list
 *                            指针列表中的每一个指针分别指向一个子串
 * @example 
 *             input=> s = "abc,123,456,,fb$"  
 *             separator = ",$"
 *            
 *             output=>s = abc'\0' 123'\0' 456'\0' '\0' fb'\0''\0'
 *             list[0] = "abc"
 *             list[1] = "123"
 *             list[2] = "456"
 *             list[3] = ""
 *             list[4] = "fb"
 *             list[5] = ""
 *
 * @param[in] str             - 源字符串
 * @param[in] separator       - 分隔字符串 
 * @param[in] list            - 字符串指针列表
 * @param[in] len             - 列表长度
 * @return    list指针列表项数，如上例所示则返回6
 **/  
int strsplit(char *s, const char *separator, char *list[],  int len)
{
    size_t count = 0;      
    if (s == NULL || list == NULL || len == 0) 
        return 0;     
        
    list[count++] = s;    
    while(*s && count < len)                   
    {       
        if (strchr(separator, *s) != NULL)
        {
            *s = '\0';                                       
            list[count++] = s + 1;                           /*指向下一个子串*/
        }
        s++;        
    }    
    return count;
}

/**
 * @brief       字符串合并(函数strsplit逆操作,即将地址连续的子串重新连接成一
 *                          个长串)
 * @example     input =>aaa'\0'bbb'\0'ccc'\0', connector = ','
 *              output=> "aaa,bbb,ccc" 
 * @param[in]   argc      - 参数个数
 * @param[in]   argv      - 参数列表
 * @param[in]   start     - 开始参数
 * @param[in]   end       - 结点参数 
 * @param[in]   connector - 连接符  
 * @return      分割后的参数个数
 * @attention   argv      - 指向的所有子串必须是指向同一片连续内存
 */  
int strmerge(int argc, char *argv[], int start, int end, char connector)
{
    int i,j;
    if (start >= end || end > argc - 1)return argc;    
    for (i = start + 1; i <= end; i++) {        
        argv[i][-1] = connector;                        /*连接子串 -----------*/        
    }
    for (i = start + 1,j = end + 1 ; j < argc; i++,j++) {
        argv[i] = argv[j];                              /*覆盖已连接的子串 ---*/
    }
    return argc - (end - start);
}

/**
 * @brief       字符串裁剪  - 在源字符串查找出所有由trim_chars指定的字符并去掉 
 * @example     input=>"adabbccababdd";
 *              strtrim(input, "ab"); 
 *              output =>"dccdd"
 * @param[in]   s               - 源字符串
 * @param[in]   trim_chars      - 待移除的分隔字符串 
 * @return      裁剪后的字符串
 */  
char *strtrim(char *s, const char *trim_chars)
{   
    char *p, *q;    
    p = q = s;
    while (*p)
    {               
        if (strchr(trim_chars, *p) == NULL)   
            *q++ = *p;
        p++;
    }
    *q = '\0';  
    return s;
}

/**
 * @brief       跳过指定的字符
 * @example     input=>"\r \n&& \r\r\nAb\r\babbccababdd";
 *              strskip(input, "\r\n &"); 
 *              output =>"Ab\r\babbccababdd"
 * @param[in]   s           - 输入串
 * @param[in]   skip_chars  - 前导字符串     
 * @return      过滤skip_chars字符后的串
 */
const char *strskip(const char *s, const char *skip_chars)
{
    while (strchr(skip_chars, *s))
        s++;
    
    return s;
}

/**
 * @brief       查找字符串在内存中首次出现的位置
 * @param[in]   mem - 内存缓冲区
 * @param[in]   len - 查找长度
 * @param[in]   str - 待查找串
 * @return      字符串在内存中首先出现的位置(如果没有找到则返回NULL)
 */
char *memstr(void *mem, int len, char *str)
{
    char *p = (char *)mem;
    char *end   = (char *)((char *)mem + len);    
    char *s1, *s2;
    char *ret = NULL;
    while (len--)
    {
        s1 = p;   
        s2 = str;
        while (s1 < end && *s2 && *s1 == *s2)
            s1++, s2++;
        
        if (!*s2) 
            return p;
        
        p++;        
    }
    return ret;
}

/**
 * @brief       查找字符串在内存中最后出现的位置
 * @param[in]   mem - 内存缓冲区
 * @param[in]   len - 查找长度
 * @param[in]   str - 待查找串
 * @return      字符串在内存中最后出现的位置(如果没有找到则返回NULL)
 */
char *memrstr(void *mem, int len, char *str)
{
    char *p = (char *)mem;
    char *end   = (char *)((char *)mem + len);    
    char *s1, *s2;
    char *ret = NULL;
    while (len--)
    {
        s1 = p;   
        s2 = str;
        while (s1 < end && *s2 && *s1 == *s2)
            s1++, s2++;

        if (!*s2)
        {        
            ret = p;                
            p   = s1;             
            continue;
        }
        p++;        
    }
    return ret;
}

