#ifndef _XY_CTYPE_H_
#define _XY_CTYPE_H_

#define xy_islower(c) (c >= 'a' && c <= 'z')
#define xy_isupper(c) (c >= 'A' && c <= 'Z')
#define xy_isdigit(c) (c >= '0' && c <= '9')
#define xy_isalnum(c) (xy_islower(c) || xy_isupper(c))
#define xy_isalpha(c) (xy_islower(c) || xy_isupper(c))
#define xy_isblank(c) (c == ' ' || c == '\t')
#define xy_iscntrl(c) (c >= 0 && c < 32)
#define xy_isgraph(c) (c >= 33 && c <= 126)
#define xy_isprint(c) (c >= 32 && c <= 126)
#define xy_ispunct(c) (c >= 33 && c <= 47)
#define xy_isspace(c) (c == ' ' || c == '\t' || c == '\n')
#define xy_isxdigit(c) \
    (xy_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define xy_tolower(c) (c >= 'A' && c <= 'Z' ? c + 32 : c)
#define xy_toupper(c) (c >= 'a' && c <= 'z' ? c - 32 : c)
#define xy_isascii(c) (c >= 0 && c <= 127)

#endif
