#include "xy_string.h"
#include <stdlib.h>

#if MINIMIZE_CATTR_TABLE
#define CATTR_TBL_SIZE 128
#else
#define CATTR_TBL_SIZE 256
#endif

//
//! @brief Table for quick lookup of character attributes.
//
//*****************************************************************************
const uint8_t am_cattr[CATTR_TBL_SIZE] = {
    XY_CATTR_NONE,                                        /* 0x00 */
    XY_CATTR_NONE,                                        /* 0x01 */
    XY_CATTR_NONE,                                        /* 0x02 */
    XY_CATTR_NONE,                                        /* 0x03 */
    XY_CATTR_NONE,                                        /* 0x04 */
    XY_CATTR_NONE,                                        /* 0x05 */
    XY_CATTR_NONE,                                        /* 0x06 */
    XY_CATTR_NONE,                                        /* 0x07 */
    XY_CATTR_NONE,                                        /* 0x08 */
    XY_CATTR_WHSPACE,                                     /* 0x09 */
    XY_CATTR_WHSPACE,                                     /* 0x0A */
    XY_CATTR_WHSPACE,                                     /* 0x0B */
    XY_CATTR_WHSPACE,                                     /* 0x0C */
    XY_CATTR_WHSPACE,                                     /* 0x0D */
    XY_CATTR_NONE,                                        /* 0x0E */
    XY_CATTR_NONE,                                        /* 0x0F */
    XY_CATTR_NONE,                                        /* 0x00 */
    XY_CATTR_NONE,                                        /* 0x11 */
    XY_CATTR_NONE,                                        /* 0x12 */
    XY_CATTR_NONE,                                        /* 0x13 */
    XY_CATTR_NONE,                                        /* 0x14 */
    XY_CATTR_NONE,                                        /* 0x15 */
    XY_CATTR_NONE,                                        /* 0x16 */
    XY_CATTR_NONE,                                        /* 0x17 */
    XY_CATTR_NONE,                                        /* 0x18 */
    XY_CATTR_NONE,                                        /* 0x19 */
    XY_CATTR_NONE,                                        /* 0x1A */
    XY_CATTR_NONE,                                        /* 0x1B */
    XY_CATTR_NONE,                                        /* 0x1C */
    XY_CATTR_NONE,                                        /* 0x1D */
    XY_CATTR_NONE,                                        /* 0x1E */
    XY_CATTR_NONE,                                        /* 0x1F */
    XY_CATTR_WHSPACE,                                     /* 0x20, space */
    XY_CATTR_FILENM83,                                    /* 0x21, ! */
    XY_CATTR_NONE,                                        /* 0x22, " */
    XY_CATTR_FILENM83,                                    /* 0x23, # */
    XY_CATTR_FILENM83,                                    /* 0x24, $ */
    XY_CATTR_FILENM83,                                    /* 0x25, % */
    XY_CATTR_FILENM83,                                    /* 0x26, & */
    XY_CATTR_FILENM83,                                    /* 0x27, ' */
    XY_CATTR_FILENM83,                                    /* 0x28, ( */
    XY_CATTR_FILENM83,                                    /* 0x29, ) */
    XY_CATTR_NONE,                                        /* 0x2A, * */
    XY_CATTR_NONE,                                        /* 0x2B, + */
    XY_CATTR_NONE,                                        /* 0x2C, , */
    XY_CATTR_FILENM83,                                    /* 0x2D, - */
    XY_CATTR_FILENM83,                                    /* 0x2E, . */
    XY_CATTR_NONE,                                        /* 0x2F, / */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x30, 0 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x31, 1 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x32, 2 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x33, 3 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x34, 4 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x35, 5 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x36, 6 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x37, 7 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x38, 8 */
    XY_CATTR_DIGIT | XY_CATTR_XDIGIT | XY_CATTR_FILENM83, /* 0x39, 9 */
    XY_CATTR_NONE,                                        /* 0x3A, : */
    XY_CATTR_NONE,                                        /* 0x3B, ; */
    XY_CATTR_NONE,                                        /* 0x3C, < */
    XY_CATTR_NONE,                                        /* 0x3D, = */
    XY_CATTR_NONE,                                        /* 0x3E, > */
    XY_CATTR_NONE,                                        /* 0x3F, ? */
    XY_CATTR_FILENM83,                                    /* 0x40, @ */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x41, A */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x42, B */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x43, C */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x44, D */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x45, E */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83,                             /* 0x46, F */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x47, G */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x48, H */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x49, I */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4A, J */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4B, K */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4C, L */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4D, M */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4E, N */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x4F, O */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x50, P */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x51, Q */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x52, R */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x53, S */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x54, T */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x55, U */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x56, V */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x57, W */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x58, X */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x59, Y */
    XY_CATTR_ALPHA | XY_CATTR_UPPER | XY_CATTR_FILENM83, /* 0x5A, Z */
    XY_CATTR_NONE,                                       /* 0x5B, [ */
    XY_CATTR_NONE,                                       /* 0x5C, \ */
    XY_CATTR_NONE,                                       /* 0x5D, ] */
    XY_CATTR_FILENM83,                                   /* 0x5E, ^ */
    XY_CATTR_FILENM83,                                   /* 0x5F, _ */
    XY_CATTR_FILENM83,                                   /* 0x60, ` */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x61, a */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x62, b */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x63, c */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x64, d */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83, /* 0x65, e */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_XDIGIT
        | XY_CATTR_FILENM83,                             /* 0x66, f */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x67, g */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x68, h */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x69, i */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6A, j */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6B, k */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6C, l */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6D, m */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6E, n */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x6F, o */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x70, p */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x71, q */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x72, r */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x73, s */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x74, t */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x75, u */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x76, v */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x77, w */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x78, x */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x79, y */
    XY_CATTR_ALPHA | XY_CATTR_LOWER | XY_CATTR_FILENM83, /* 0x7A, z */
    XY_CATTR_FILENM83,                                   /* 0x7B, { */
    XY_CATTR_NONE,                                       /* 0x7C, | */
    XY_CATTR_FILENM83,                                   /* 0x7D, } */
    XY_CATTR_FILENM83,                                   /* 0x7E, ~ */
    XY_CATTR_NONE                                        /* 0x7F, delete */

    //
    // All bit7 chars are XY_CATTR_NONE.
    //
};


void xy_memset(void *dst, uint8_t val, uint32_t len)
{
    uint8_t *p = dst;
    while (len--) {
        *p++ = val;
    }
}

int32_t xy_memcmp(const void *s1, const void *s2, uint32_t n)
{
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }

    return 0;
}

void *xy_memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t *p1       = dst;
    const uint8_t *p2 = src;
    while (n--) {
        *p1++ = *p2++;
    }
    return dst;
}

uint32_t xy_strlen(const char *str)
{
    const char *p;

    for (p = str; *p; ++p)
        ;

    return (p - str);
}

int32_t xy_strncmp(const char *str1, const char *str2, uint32_t num)
{
    while (num--) {
        // Check for inequality OR end of string
        if (*str1 != *str2 || *str1 == '\0') {
            return *str1 - *str2;
        }

        str1++;
        str2++;
    }

    return 0;
}


int32_t xy_strcmp(const char *str1, const char *str2)
{
    return xy_strncmp(str1, str2, 0xffffffff);
}

/**
 * Compare two strings with case-insensitivity.
 */
int32_t xy_stricmp(const char *str1, const char *str2)
{
    uint8_t cChar1, cChar2;

    while (*str1 && *str2) {
        cChar1 = *str1++;
        cChar2 = *str2++;

        cChar1 |= (am_cattr[cChar1] & XY_CATTR_UPPER) ? 0x20 : 0x00;
        cChar2 |= (am_cattr[cChar2] & XY_CATTR_UPPER) ? 0x20 : 0x00;

        if (cChar1 != cChar2) {
            return cChar1 - cChar2;
        }
    }

    return *str1 - *str2;
}


char *xy_strncpy(char *dest, const char *src, uint32_t n)
{
    char *p = dest;
    if (!dest || !src) {
        return NULL;
    }

    while (n-- && (*p++ = *src++)) {
        // 空循环体，完成逐字符拷贝
    }

    return dest;
}

//*****************************************************************************
//
// 安全字符串拷贝实现（类似strcpy）
//
//*****************************************************************************
char *xy_strcpy(char *dest, const char *src)
{
    return xy_strncpy(dest, src, 0xffffffff);
}

char *xy_strchr(const char *str, uint8_t c)
{
    if (!str)
        return NULL;
    while (*str) {
        if (*str == (char)c) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

char *xy_strrchr(const char *str, uint8_t c)
{
    const char *last = NULL;
    while (*str) {
        if (*str == c) {
            last = str;
        }
        str++;
    }
    return (char *)last;
}

size_t xy_strcspn(const char *str1, const char *str2)
{
    if (!str1 || !str2)
        return 0; /* 与标准库语义不同: 返回0 而非崩溃 */
    const char *p1;
    const char *p2;
    for (p1 = str1; *p1; ++p1) {
        for (p2 = str2; *p2; ++p2) {
            if (*p1 == *p2) {
                return (size_t)(p1 - str1);
            }
        }
    }
    return (size_t)(p1 - str1);
}

int32_t xy_strpbrk(const char *str1, const char *str2)
{
    if (!str1 || !str2)
        return -1;
    const char *p1;
    const char *p2;
    for (p1 = str1; *p1; ++p1) {
        for (p2 = str2; *p2; ++p2) {
            if (*p1 == *p2) {
                return (int32_t)(p1 - str1);
            }
        }
    }
    return -1;
}

char *xy_strstr(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return NULL;
    if (!*needle)
        return (char *)haystack;
    const char *h = haystack;
    while (*h) {
        if (*h == *needle) {
            const char *h2 = h;
            const char *n2 = needle;
            while (*h2 && *n2 && *h2 == *n2) {
                ++h2;
                ++n2;
            }
            if (!*n2) {
                return (char *)h;
            }
        }
        ++h;
    }
    return NULL;
}

char *xy_strtok(char *str, const char *delim)
{
    /* 简化版 strtok：与标准行为接近，使用静态游标，不支持多线程 */
    static char *save_ptr;
    if (delim == NULL)
        return NULL;
    if (str) {
        save_ptr = str;
    } else if (!save_ptr) {
        return NULL;
    }
    /* 跳过分隔符 */
    char *start = save_ptr;
    while (*start && xy_strpbrk(start, delim) == 0) {
        /* xy_strpbrk 返回第一个匹配位置索引,
         * 若为0说明第一个字符就是分隔符，需要继续 */
        if (xy_strpbrk(start, delim) == 0) { /* 当前字符是分隔符 */
            ++start;
        } else {
            break;
        }
    }
    while (*start) {
        int idx = xy_strpbrk(start, delim);
        if (idx == 0) { /* 当前即分隔符 */
            ++start;
            continue;
        }
        break;
    }
    if (!*start) {
        save_ptr = NULL;
        return NULL;
    }
    char *token = start;
    /* 找到下一个分隔符位置 */
    char *scan = start;
    while (*scan) {
        int off = xy_strpbrk(scan, delim);
        if (off == 0) { /* 分隔符 */
            *scan    = '\0';
            save_ptr = scan + 1;
            return token;
        }
        ++scan;
    }
    save_ptr = NULL;
    return token;
}

/** Convert a string of characters representing
 * a hex buffer into a series of bytes of that real value*/
uint8_t *hexstr2bytes(char *hexstr)
{
    if (!hexstr)
        return NULL;

    size_t len = xy_strlen(hexstr);
    if (len == 0 || len % 2 != 0)
        return NULL; /* Hex string must have even length */

    size_t byte_len = len / 2;
    uint8_t *bytes  = (uint8_t *)malloc(byte_len);
    if (!bytes)
        return NULL;

    for (size_t i = 0; i < byte_len; i++) {
        char high = hexstr[i * 2];
        char low  = hexstr[i * 2 + 1];

        uint8_t h_val, l_val;

        /* Convert high nibble */
        if (high >= '0' && high <= '9')
            h_val = high - '0';
        else if (high >= 'a' && high <= 'f')
            h_val = high - 'a' + 10;
        else if (high >= 'A' && high <= 'F')
            h_val = high - 'A' + 10;
        else {
            free(bytes);
            return NULL;
        }

        /* Convert low nibble */
        if (low >= '0' && low <= '9')
            l_val = low - '0';
        else if (low >= 'a' && low <= 'f')
            l_val = low - 'a' + 10;
        else if (low >= 'A' && low <= 'F')
            l_val = low - 'A' + 10;
        else {
            free(bytes);
            return NULL;
        }

        bytes[i] = (h_val << 4) | l_val;
    }

    return bytes;
}

/* ========================================================================
 * Additional String Functions Implementation
 * ======================================================================== */

/**
 * @brief Move memory area (handles overlapping regions)
 */
void *xy_memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) {
        return dest;
    }

    /* Check if regions overlap */
    if (d < s) {
        /* Copy forward */
        while (n--) {
            *d++ = *s++;
        }
    } else {
        /* Copy backward to handle overlap */
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }

    return dest;
}

/**
 * @brief Scan memory for a character
 */
void *xy_memchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s;
    uint8_t ch = (uint8_t)c;

    while (n--) {
        if (*p == ch) {
            return (void *)p;
        }
        p++;
    }

    return NULL;
}

/**
 * @brief Reverse memory search for a character
 */
void *xy_memrchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s + n;
    uint8_t ch = (uint8_t)c;

    while (n--) {
        if (*--p == ch) {
            return (void *)p;
        }
    }

    return NULL;
}

/**
 * @brief Compare strings ignoring case
 */
int xy_strcasecmp(const char *s1, const char *s2)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    uint8_t c1, c2;

    while (*p1 && *p2) {
        c1 = *p1;
        c2 = *p2;

        /* Convert to lowercase */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;

        if (c1 != c2) {
            return c1 - c2;
        }

        p1++;
        p2++;
    }

    c1 = *p1;
    c2 = *p2;
    if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z') c2 += 32;

    return c1 - c2;
}

/**
 * @brief Compare strings ignoring case (limited length)
 */
int xy_strncasecmp(const char *s1, const char *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    uint8_t c1, c2;

    if (n == 0) {
        return 0;
    }

    while (n-- && *p1 && *p2) {
        c1 = *p1;
        c2 = *p2;

        /* Convert to lowercase */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;

        if (c1 != c2) {
            return c1 - c2;
        }

        p1++;
        p2++;
    }

    if (n == 0) {
        return 0;
    }

    c1 = *p1;
    c2 = *p2;
    if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z') c2 += 32;

    return c1 - c2;
}

/**
 * @brief Get string length (with maximum limit)
 */
size_t xy_strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;

    while (len < maxlen && s[len]) {
        len++;
    }

    return len;
}

/**
 * @brief Duplicate a string (allocates memory)
 * @note Requires xy_malloc to be implemented
 */
char *xy_strdup(const char *s)
{
    size_t len;
    char *new_str;

    if (s == NULL) {
        return NULL;
    }

    len = xy_strlen(s) + 1;  /* +1 for null terminator */

    /* Note: Will be implemented when xy_malloc is available */
    /* Placeholder implementation */
    (void)new_str;
    (void)len;
    return NULL;
}

/**
 * @brief Duplicate a string with length limit (allocates memory)
 * @note Requires xy_malloc to be implemented
 */
char *xy_strndup(const char *s, size_t n)
{
    size_t len;
    char *new_str;

    if (s == NULL) {
        return NULL;
    }

    len = xy_strnlen(s, n);

    /* Note: Will be implemented when xy_malloc is available */
    /* Placeholder implementation */
    (void)new_str;
    (void)len;
    return NULL;
}

/**
 * @brief Get span of character set in string
 */
size_t xy_strspn(const char *s, const char *accept)
{
    const char *p;
    const char *a;
    size_t count = 0;

    for (p = s; *p; p++) {
        for (a = accept; *a; a++) {
            if (*p == *a) {
                break;
            }
        }
        if (*a == '\0') {
            return count;
        }
        count++;
    }

    return count;
}
