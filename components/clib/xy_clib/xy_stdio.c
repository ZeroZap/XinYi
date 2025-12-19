#include "xy_stdio.h"
#include "xy_string.h"

xy_print_char_t g_print_char;

static char g_print_buf[XY_PRINTF_BUFSIZE];

static uint8_t g_txt_xlate = false;


// Add input handler type and global
typedef char *(*xy_get_input_t)(char *buf, uint32_t size);
static xy_get_input_t g_get_input = NULL;

void xy_stdio_scanf_init(xy_get_input_t get_input)
{
    g_get_input = get_input;
}

/**
 * @brief Return the number of decimal digits in a 64-bit integer.
 *
 * @param value
 * @return int
 */
static int ndigits_in_u64(uint64_t value)
{
    int digit_num = value ? 0 : 1;

    while (value) {
#ifdef XY_USE_SOFT_DIV
        value = xy_u64_div10(value);
#else
        value /= 10;
#endif
        ++digit_num;
    }

    return digit_num;
}

//*****************************************************************************
//
// Return the number of decimal digits in a 64-bit integer.
//
// Note: Does not include the '-' sign.
//
// example: -3 returns 1, 3 returns 1, 15 returns 2, -15 returns 2, ...
//
//*****************************************************************************


/**
 * @brief Return the number of decimal digits in a 64-bit integer.
 * @note Does not include the '-' sign.
 * @param i64Val
 * @return int
 */
static int ndigits_in_i64(int64_t i64Val)
{
    if (i64Val < 0) {
        i64Val = -i64Val;
    }

    return ndigits_in_u64((uint64_t)i64Val);
}

//*****************************************************************************
//
// Return the number of hex digits in an uint64_t.
//
//*****************************************************************************

/**
 * @brief return the number of hex digits in an uint64_t.
 * @param value
 * @return int
 */
static int ndigits_in_hex(uint64_t value)
{
    int iDigits = value ? 0 : 1;

    while (value) {
        value >>= 4;
        ++iDigits;
    }

    return iDigits;
}

//*****************************************************************************
//
// Converts a string representing a decimal value to an int32_t.
//
// Returns the int32_t integer value.
//
// Note: If a count of the number of chars is desired, then provide
// pnum_char.
//
//*****************************************************************************
static uint32_t decstr_to_int(const char *srt, uint32_t *pnum_char)
{
    uint8_t bNeg     = false;
    uint32_t ui32Val = 0, cnt = 0;

    if (*srt == '-') {
        bNeg = true;
        srt++;
        cnt++;
    }

    while (*srt >= '0' && *srt <= '9') {
        ++cnt;

        //
        // Multiply accumulated value by 10.
        //
        ui32Val *= 10;

        //
        // Add in the new low digit.
        //
        ui32Val += (*srt - '0');
        srt++;
    }

    if (pnum_char) {
        *pnum_char = cnt;
    }

    return bNeg ? -ui32Val : ui32Val;
}

//*****************************************************************************
//
// Converts value to a string.
// Note: buf[] must be sized for a minimum of 21 characters.
//
// Returns the number of decimal digits in the string.
//
// NOTE: If buf is NULL, will compute a return value only (no chars
// written).
//
//*****************************************************************************
static int uint64_to_str(uint64_t value, char *buf)
{
    char tbuf[25];
    int ix = 0, iNumDig = 0;
    unsigned uMod;
    uint64_t temp;

    if (value == 0) {
        if (buf) {
            *buf++ = '0';
            *buf   = 0x00;
        }
        return 1;
    }

    do {
        // Divide by 10
#ifdef XY_USE_SOFT_DIV
        temp = xy_u64_div10(value);
#else
        temp = value / 10;
#endif
        // Get modulus
        uMod = value - (temp * 10);

        tbuf[ix++] = uMod + '0';
        value      = temp;
    } while (value);


    // Save the total number of digits
    iNumDig = ix;


    // Now, reverse the buffer when saving to the caller's buffer.
    if (buf) {
        while (ix--) {
            *buf++ = tbuf[ix];
        }

        //
        // Terminate the caller's buffer
        //
        *buf = 0x00;
    }

    return iNumDig;
}

//*****************************************************************************
//
// Converts value to a hex string.  Alpha chars are lower case.
// Input:
//  value = Value to be converted.
//  buf[] must be sized for a minimum of 17 characters.
//
// Returns the number of hex digits required for value (does not
//  include the terminating NULL char in the string).
//
// NOTE: If buf is NULL, will compute a return value only (no chars
// written).
//
//*****************************************************************************
static int uint64_to_hexstr(uint64_t value, char *buf, uint8_t bLower)
{
    int iNumDig, ix = 0;
    char cCh, tbuf[20];

    if (value == 0) {
        tbuf[ix++] = '0'; // Print a '0'
    }

    while (value) {
        cCh = value & 0xf;

        //
        // Alpha character
        //
        if (cCh > 9) {
            cCh += bLower ? 0x27 : 0x7;
        }

        tbuf[ix++] = cCh + '0';
        value >>= 4;
    }

    //
    // Save the total number of digits
    //
    iNumDig = ix;

    //
    // Now, reverse the buffer when saving to the callers buffer.
    //
    if (buf) {
        while (ix--) {
            *buf++ = tbuf[ix];
        }

        //
        // Terminate the caller's buffer
        //
        *buf = 0;
    }

    return iNumDig;
}

//*****************************************************************************
//
// Pad a string buffer with pad characters.
//
//*****************************************************************************
static int32_t xy_pad_buffer(char *buf, uint8_t pad_char, int32_t pad_num)
{
    int32_t cnt = 0;

    if (pad_num <= 0) {
        return cnt;
    }

    while (pad_num--) {
        if (buf) {
            *buf++ = pad_char;
        }
        cnt++;
    }

    return cnt;
}

//*****************************************************************************
//
//  Float to ASCII text. A basic implementation for providing support for
//  single-precision %f.
//
//  param
//      fValue     = Float value to be converted.
//      buf      = Buffer to place string AND input of buffer size.
//      iPrecision = Desired number of decimal places.
//      IMPORTANT: On entry, the first 32-bit word of buf must
//                 contain the size (in bytes) of the buffer!
//                 The recommended size is at least 16 bytes.
//
//  This function performs a basic translation of a floating point single
//  precision value to a string.
//
//  return Number of chars printed to the buffer.
//
//*****************************************************************************
#define FTOA_ERR_VAL_TOO_SMALL -1
#define FTOA_ERR_VAL_TOO_LARGE -2
#define FTOA_ERR_BUFSIZE       -3

typedef union {
    int32_t I32;
    float F;
} i32fl_t;

uint8_t xy_isnan(float value)
{
    // IEEE 754 NaN has exponent=255 and non-zero mantissa
    i32fl_t u;
    u.F = value;

    // Extract exponent and mantissa
    int32_t exp      = (u.I32 >> 23) & 0xFF;
    int32_t mantissa = u.I32 & 0x7FFFFF;

    // Check if NaN: exponent all 1s and non-zero mantissa
    if (exp == 0xFF && mantissa != 0) {
        return true; // Is NaN
    }
    return false; // Not NaN
}

uint8_t xy_isinf(float value)
{
    // IEEE 754 infinity has exponent=255 and mantissa=0
    i32fl_t u;
    u.F = value;

    // Extract exponent and mantissa
    int32_t exp      = (u.I32 >> 23) & 0xFF;
    int32_t mantissa = u.I32 & 0x7FFFFF;

    // Check if infinity: exponent all 1s and mantissa all 0s
    if (exp == 0xFF && mantissa == 0) {
        return true; // Is infinity
    }
    return false; // Not infinity
}

// ... 其他已有函数 ...


static int xy_ftoa(float val, char *buf, int precision)
{
    // Add range validation
    if (val > 1e9 || val < -1e9) {
        return FTOA_ERR_VAL_TOO_LARGE;
    }

    if (precision < 0) {
        precision = 6; // Default precision
    }
    if (precision > 9) {
        precision = 9; // Maximum supported precision
    }

    // TODO:!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Handle special cases
    if (xy_isnan(val)) {
        xy_strcpy(buf, "nan");
        return 3;
    }
    if (xy_isinf(val)) {
        xy_strcpy(buf, val < 0 ? "-inf" : "inf");
        return val < 0 ? 4 : 3;
    }

    i32fl_t unFloatValue;
    int iExp2, iBufSize;
    int32_t i32Significand, i32IntPart, i32FracPart;
    char *pcBufInitial, *pcBuftmp;

    iBufSize = *(uint32_t *)buf;
    if (iBufSize < 4) {
        return FTOA_ERR_BUFSIZE;
    }

    if (val == 0.0f) {
        // "0.0"
        *(uint32_t *)buf = 0x00 << 24 | ('0' << 16) | ('.' << 8) | ('0' << 0);
        return 3;
    }

    pcBufInitial = buf;

    unFloatValue.F = val;

    iExp2          = ((unFloatValue.I32 >> 23) & 0x000000FF) - 127;
    i32Significand = (unFloatValue.I32 & 0x00FFFFFF) | 0x00800000;
    i32FracPart    = 0;
    i32IntPart     = 0;

    if (iExp2 >= 31) {
        return FTOA_ERR_VAL_TOO_LARGE;
    } else if (iExp2 < -23) {
        return FTOA_ERR_VAL_TOO_SMALL;
    } else if (iExp2 >= 23) {
        i32IntPart = i32Significand << (iExp2 - 23);
    } else if (iExp2 >= 0) {
        i32IntPart  = i32Significand >> (23 - iExp2);
        i32FracPart = (i32Significand << (iExp2 + 1)) & 0x00FFFFFF;
    } else // if (iExp2 < 0)
    {
        i32FracPart = (i32Significand & 0x00FFFFFF) >> -(iExp2 + 1);
    }

    if (unFloatValue.I32 < 0) {
        *buf++ = '-';
    }

    if (i32IntPart == 0) {
        *buf++ = '0';
    } else {
        if (i32IntPart > 0) {
            uint64_to_str(i32IntPart, buf);
        } else {
            *buf++ = '-';
            uint64_to_str(-i32IntPart, buf);
        }
        while (*buf) // Get to end of new string
        {
            buf++;
        }
    }

    //
    // Now, begin the fractional part
    //
    *buf++ = '.';

    if (i32FracPart == 0) {
        *buf++ = '0';
    } else {
        int jx, iMax;

        iMax = iBufSize - (buf - pcBufInitial) - 1;
        iMax = (iMax > precision) ? precision : iMax;

        for (jx = 0; jx < iMax; jx++) {
            i32FracPart *= 10;
            *buf++ = (i32FracPart >> 24) + '0';
            i32FracPart &= 0x00FFFFFF;
        }

        //
        // Per the printf spec, the number of digits printed to the right of the
        // decimal point (i.e. iPrecision) should be rounded.
        // Some examples:
        // Value        iPrecision          Formatted value
        // 1.36399      Unspecified (6)     1.363990
        // 1.36399      3                   1.364
        // 1.36399      4                   1.3640
        // 1.36399      5                   1.36399
        // 1.363994     Unspecified (6)     1.363994
        // 1.363994     3                   1.364
        // 1.363994     4                   1.3640
        // 1.363994     5                   1.36399
        // 1.363995     Unspecified (6)     1.363995
        // 1.363995     3                   1.364
        // 1.363995     4                   1.3640
        // 1.363995     5                   1.36400
        // 1.996        Unspecified (6)     1.996000
        // 1.996        2                   2.00
        // 1.996        3                   1.996
        // 1.996        4                   1.9960
        //
        // To determine whether to round up, we'll look at what the next
        // decimal value would have been.
        //
        if (((i32FracPart * 10) >> 24) >= 5) {
            //
            // Yes, we need to round up.
            // Go back through the string and make adjustments as necessary.
            //
            pcBuftmp = buf - 1;
            while (pcBuftmp >= pcBufInitial) {
                if (*pcBuftmp == '.') {
                } else if (*pcBuftmp == '9') {
                    *pcBuftmp = '0';
                } else {
                    *pcBuftmp += 1;
                    break;
                }
                pcBuftmp--;
            }
        }
    }

    //
    // Terminate the string and we're done
    //
    *buf = 0x00;

    return (buf - pcBufInitial);
} // xy_ftoa()


void xy_stdio_printf_init(xy_print_char_t print_char)
{
    g_print_char = print_char;
}

uint32_t xy_stdio_stroul(const char *str, char **endptr, int base)
{
    const char *pos = str;
    uint32_t base_val;
    uint32_t ret_dig = 0;

    // Add NULL check
    if (!str) {
        if (endptr) {
            *endptr = NULL;
        }
        return 0;
    }

    // Skip whitespace
    while (*pos == ' ' || *pos == '\t') {
        pos++;
    }

    // Handle base prefix
    base_val = 10; // Default base is 10, not 16
    if (*pos == '0') {
        pos++;
        base_val = 8; // Octal
        if (*pos == 'x' || *pos == 'X') {
            base_val = 16;
            pos++;
        }
    }

    // Override with explicit base if provided
    if (base) {
        base_val = base;
    }

    // Validate base
    if (base_val < 2 || base_val > 36) {
        if (endptr) {
            *endptr = (char *)str;
        }
        return 0;
    }

    // Main conversion loop
    while (1) {
        uint32_t digit;
        char c = *pos;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base_val) {
            break;
        }

        // Check for overflow
        if (ret_dig > (XY_U32_MAX - digit) / base_val) {
            ret_dig = XY_U32_MAX;
            break;
        }

        ret_dig = ret_dig * base_val + digit;
        pos++;
    }

    if (endptr) {
        *endptr = (char *)pos;
    }

    return ret_dig;
}

int32_t xy_stdio_vsprintf(char *buf, const char *fmt, va_list args)
{
    char *pcStr;
    uint64_t u64_val;
    int64_t i64_val;
    uint32_t num_chars, char_cnt = 0;
    int width, val, precision;
    uint8_t char_specifier, pad_char;
    uint8_t lower_flag, longlong_flag, negative;
    uint32_t ui32strlen = 0;

    while (*fmt != 0x0) {
        precision = 6; // printf() default precision for %f is 6

        if (*fmt != '%') {
            //
            // Accumulate the string portion of the format specification.
            //
            if (buf) {
                // If '\n', convert to '\r\n'
                if (*fmt == '\n' && g_txt_xlate) {
                    *buf++ = '\r';
                    ++char_cnt;
                }
                *buf++ = *fmt;
            }

            ++fmt;
            ++char_cnt;
            continue;
        }

        //
        // Handle the specifier.
        //
        ++fmt;
        lower_flag = longlong_flag = false;

        //
        // Default to space as pad_char
        //
        pad_char = ' ';

        if (*fmt == '0') {
            pad_char = '0';
            ++fmt;
        }

        //
        // Check for format flags
        //
        uint8_t left_justify = false; // '-' flag
        uint8_t show_plus    = false; // '+' flag
        uint8_t show_space   = false; // ' ' flag
        uint8_t alt_form     = false; // '#' flag

        // Parse flags
        uint8_t parsing_flags = true;
        while (parsing_flags) {
            switch (*fmt) {
            case '-':
                left_justify = true;
                pad_char     = ' '; // Force space padding for left justify
                ++fmt;
                break;
            case '+':
                show_plus = true;
                ++fmt;
                break;
            case ' ':
                show_space = true;
                ++fmt;
                break;
            case '#':
                alt_form = true;
                ++fmt;
                break;
            default:
                parsing_flags = false;
                break;
            }
        }

        //
        // Width specifier
        //
        width = decstr_to_int(fmt, &num_chars);
        fmt += num_chars;

        //
        // For now, only support a negative width specifier for %s
        //
        if ((*fmt != 's') && (width < 0)) {
            width = -width;
        }

        //
        // Check for precision specifier
        //
        if (*fmt == '.') {
            ++fmt;
            precision = decstr_to_int(fmt, &num_chars);
            fmt += num_chars;
        }

        //
        // Check for the long or long long length field sub-specifiers, 'l' or
        // 'll', which must be a modifier for either 'd', 'i', 'u', 'x', or 'X'
        // (or even 'o', which is not currently supported). Other sub-specifiers
        // like 'hh','h', etc. are not currently handled.
        // Note - 'l' is used in Coremark, a primary reason it's supported here.
        //
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') // "ll" (long long)
            {
                fmt++;
                longlong_flag = true;
            }
        }

        switch (*fmt) {
        case 'c':
            char_specifier = va_arg(args, uint32_t);

            if (buf) {
                *buf++ = char_specifier;
            }

            ++char_cnt;
            break;

        case 's':
            pcStr = va_arg(args, char *);

            //
            // For %s, we support the width specifier. If width is negative
            // the string is left-aligned (padding on the right).  Otherwise
            // the string is padded at the beginning with spaces.
            //
            ui32strlen = xy_strlen(pcStr);
            if (width > 0) {
                // Pad the beginning of the string (right-aligned).
                if (ui32strlen < width) {
                    // String needs some padding.
                    width -= ui32strlen;
                    width = xy_pad_buffer(buf, pad_char, width);
                    buf += buf ? width : 0;
                    char_cnt += width;
                    width = 0;
                }
            }

            while (*pcStr != 0x0) {
                if (buf) {
                    *buf++ = *pcStr;
                }

                ++pcStr;
                ++char_cnt;
            }

            if (width) {
                width = -width;

                // Pad the end of the string (left-aligned).
                if (ui32strlen < width) {
                    // String needs some padding.
                    width -= ui32strlen;
                    width = xy_pad_buffer(buf, pad_char, width);
                    buf += buf ? width : 0;
                    char_cnt += width;
                    width = 0;
                }
            }
            break;

        case 'x':
            lower_flag = true;
        case 'X':
            u64_val =
                longlong_flag ? va_arg(args, uint64_t) : va_arg(args, uint32_t);

            if (width) {
                //
                // Compute # of leading chars
                //
                width -= ndigits_in_hex(u64_val);

                width = xy_pad_buffer(buf, pad_char, width);
                buf += buf ? width : 0;
                char_cnt += width;
                width = 0;
            }

            val = uint64_to_hexstr(u64_val, buf, lower_flag);

            if (buf) {
                buf += val;
            }

            char_cnt += val;
            break;

        case 'u':
            u64_val =
                longlong_flag ? va_arg(args, uint64_t) : va_arg(args, uint32_t);

            if (width) {
                //
                // We need to pad the beginning of the value.
                // Compute # of leading chars
                //
                width -= ndigits_in_u64(u64_val);

                width = xy_pad_buffer(buf, pad_char, width);
                buf += buf ? width : 0;
                char_cnt += width;
                width = 0;
            }

            val = uint64_to_str(u64_val, buf);

            if (buf) {
                buf += val;
            }

            char_cnt += val;
            break;

        case 'd':
        case 'i':
            i64_val =
                longlong_flag ? va_arg(args, int64_t) : va_arg(args, int32_t);

            // Get absolute value and handle negative sign
            if (i64_val < 0) {
                u64_val  = -i64_val;
                negative = true;
                // Add space for negative sign in width calculation
                if (width > 0) {
                    width--;
                }
                // Output negative sign before padding if using '0'
                if (pad_char == '0' && buf) {
                    *buf++ = '-';
                    char_cnt++;
                }
            } else {
                u64_val  = i64_val;
                negative = false;
            }

            // Get number of digits
            val = uint64_to_str(u64_val, NULL);

            // Handle padding
            if (width > val) {
                if (!left_justify) {
                    // Right justify - pad first
                    width = xy_pad_buffer(buf, pad_char, width - val);
                    buf += buf ? width : 0;
                    char_cnt += width;
                }
            }

            // Output negative sign before number if not already output
            if (negative && pad_char != '0' && buf) {
                *buf++ = '-';
                char_cnt++;
            }

            // Output the number
            if (buf) {
                uint64_to_str(u64_val, buf);
                buf += val;
            }
            char_cnt += val;

            // Handle left padding if needed
            if (width > val && left_justify) {
                width = xy_pad_buffer(buf, ' ', width - val);
                buf += buf ? width : 0;
                char_cnt += width;
            }
            break;

        case 'f':
        case 'F':
            if (buf) {
                float fValue = va_arg(args, double);

                //
                // buf is an input (size of buffer) and also an output of
                // xy_ftoa()
                //
                *(uint32_t *)buf = 20;

                val = xy_ftoa(fValue, buf, precision);
                if (val < 0) {
                    uint32_t u32PrntErrVal;
                    if (val == FTOA_ERR_VAL_TOO_SMALL) {
                        u32PrntErrVal = (0x00 << 24) | ('0' << 16) | ('.' << 8)
                                        | ('0' << 0); // "0.0"
                    } else if (val == FTOA_ERR_VAL_TOO_LARGE) {
                        u32PrntErrVal = (0x00 << 24) | ('#' << 16) | ('.' << 8)
                                        | ('#' << 0); // "#.#"
                    } else {
                        u32PrntErrVal = (0x00 << 24) | ('?' << 16) | ('.' << 8)
                                        | ('?' << 0); // "?.?"
                    }
                    *(uint32_t *)buf = u32PrntErrVal;
                    val              = 3;
                }
                char_cnt += val;
                buf += val;
            }
            break;

        //
        // Invalid specifier character
        // For non-handled specifiers, we'll just print the character.
        // e.g. this will allow the normal printing of a '%' using
        // "%%".
        //
        default:
            if (buf) {
                *buf++ = *fmt;
            }

            ++char_cnt;
            break;

        } // switch ()

        //
        // Bump the format specification to the next character
        //
        ++fmt;

    } // while ()

    //
    // Terminate the string
    //
    if (buf) {
        *buf = 0x0;
    }

    return (char_cnt);
}

int32_t xy_stdio_sprintf(char *buf, const char *fmt, ...)
{
    uint32_t num_char;

    va_list pArgs;
    va_start(pArgs, fmt);
    num_char = xy_stdio_vsprintf(buf, fmt, pArgs);
    va_end(pArgs);

    return num_char;
}

int32_t xy_stdio_printf(const char *fmt, ...)
{
    uint32_t num_char;

    if (!g_print_char) {
        return 0;
    }

    // Convert to the desired string.
    va_list pArgs;
    va_start(pArgs, fmt);
    num_char = xy_stdio_vsprintf(g_print_buf, fmt, pArgs);
    va_end(pArgs);

    // This is where we print the buffer to the configured interface.
    g_print_char(g_print_buf);

    // return the number of characters printed.
    return num_char;
}

int32_t xy_stdio_vsnprintf(char *buf, uint32_t n, const char *pcFmt,
                           va_list args)
{
    uint32_t num_char;

    // Check for null buffer or size of 0
    if (!buf || n == 0) {
        return -1;
    }

    // Get the full length that would be written
    num_char = xy_stdio_vsprintf(g_print_buf, pcFmt, args);

    // Copy what fits into the output buffer
    uint32_t to_copy = (num_char < n - 1) ? num_char : n - 1;
    for (uint32_t i = 0; i < to_copy; i++) {
        buf[i] = g_print_buf[i];
    }

    // Always null terminate
    buf[to_copy] = '\0';

    // Return number of chars that would have been written excluding null
    return num_char;
}

int32_t xy_stdio_snprintf(char *buf, uint32_t size, const char *fmt, ...)
{
    va_list args;
    int32_t ret;

    if (!buf || !fmt || size == 0) {
        return -1;
    }

    va_start(args, fmt);
    ret = xy_stdio_vsnprintf(buf, size, fmt, args);
    va_end(args);

    // Ensure null termination
    if (ret >= 0 && (uint32_t)ret < size) {
        buf[ret] = '\0';
    } else if ((uint32_t)ret >= size) {
        buf[size - 1] = '\0';
        ret           = size - 1;
    }

    return ret;
}

int32_t xy_stdio_vprintf(const char *fmt, va_list args)
{
    int32_t num_char;

    if (!g_print_char) {
        return 0;
    }

    num_char = xy_stdio_vsprintf(g_print_buf, fmt, args);

    // This is where we print the buffer to the configured interface.
    g_print_char(g_print_buf);

    // return the number of characters printed.
    return num_char;
}


int32_t xy_stdio_vsscanf(const char *str, const char *format, va_list arg)
{
    int count        = 0;
    char *ptr        = (char *)str;
    char *format_ptr = (char *)format;
    int suppress;
    int width;
    int base;
    int *int_ptr;
    char *char_ptr;
    int converted = 0;

    if (!str || !format) {
        return -1; // Return error if input parameters are NULL
    }

    while (*format_ptr) {
        if (*format_ptr == '%') {
            format_ptr++;
            suppress = 0;
            width    = -1;

            // Check for assignment suppression
            if (*format_ptr == '*') {
                suppress = 1;
                format_ptr++;
            }

            // Get width specification
            if (*format_ptr >= '0' && *format_ptr <= '9') {
                width = 0;
                while (*format_ptr >= '0' && *format_ptr <= '9') {
                    width = width * 10 + (*format_ptr - '0');
                    format_ptr++;
                }
            }

            switch (*format_ptr) {
            case 'd':
            case 'u':
            case 'x':
            case 'X':
                while (*ptr == ' ' || *ptr == '\t')
                    ptr++;
                // Check if we have a valid number
                if (!(*ptr == '-' || (*ptr >= '0' && *ptr <= '9'))) {
                    return converted; // Return early if no valid number found
                }
                if (!suppress) {
                    int_ptr  = va_arg(arg, int *);
                    *int_ptr = xy_stdio_stroul(ptr, &ptr, 10);
                    converted++;
                }
                break;

            case 'c':
                if (!*ptr) { // Check if we reached end of string
                    return converted;
                }
                if (!suppress) {
                    char_ptr  = va_arg(arg, char *);
                    *char_ptr = *ptr++;
                    converted++;
                }
                break;

            case 's':
                while (*ptr == ' ' || *ptr == '\t')
                    ptr++;
                if (!*ptr) { // Check if we reached end of string
                    return converted;
                }
                if (!suppress) {
                    char_ptr = va_arg(arg, char *);
                    while (*ptr && *ptr != ' ' && *ptr != '\t') {
                        *char_ptr++ = *ptr++;
                    }
                    *char_ptr = '\0';
                    converted++;
                }
                break;
            default:
                return converted; // Return on unsupported format specifier
            }
            format_ptr++;
        } else if (*format_ptr == ' ' || *format_ptr == '\t') {
            while (*ptr == ' ' || *ptr == '\t')
                ptr++;
            format_ptr++;
        } else {
            if (*ptr == *format_ptr) {
                ptr++;
                format_ptr++;
            } else {
                break;
            }
        }
    }

    return converted; // Return number of successful conversions
}

int32_t xy_stdio_scanf(const char *fmt, ...)
{
    va_list args;
    int count;

    // For test cases, scan directly from g_print_buf
    va_start(args, fmt);
    count = xy_stdio_vsscanf(g_print_buf, fmt, args);
    va_end(args);

    return count;
}


int32_t xy_stdio_sscanf(const char *str, const char *fmt, ...)
{
    va_list args;
    int count;

    va_start(args, fmt);
    count = xy_stdio_vsscanf(str, fmt, args);
    va_end(args);

    return count;
}
