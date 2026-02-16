# XinYi Code Style Design Guide

## Overview

This guide consolidates all code style conventions from the XinYi project. It follows RFC2119/RFC8174 standards where **MUST**, **SHOULD**, **MAY**, and **OPTIONAL** have specific meanings.

**Table of Contents**
- [1. General Rules](#1-general-rules)
- [2. Variables](#2-variables)
- [3. Functions](#3-functions)
- [4. Structures, Enumerations, and Typedefs](#4-structures-enumerations-and-typedefs)
- [5. Macros and Preprocessor](#5-macros-and-preprocessor)
- [6. Comments](#6-comments)
- [7. Compound Statements](#7-compound-statements)
- [8. Switch Statements](#8-switch-statements)
- [9. Documentation (Doxygen)](#9-documentation-doxygen)
- [10. Header and Source Files](#10-header-and-source-files)
- [11. Conditional Compilation Examples](#11-conditional-compilation-examples)
- [12. Code Organization Best Practices](#12-code-organization-best-practices)
- [13. Common Pitfalls to Avoid](#13-common-pitfalls-to-avoid)
- [14. Real-World Examples](#14-real-world-examples)
- [15. Formatting Tools](#15-formatting-tools)
- [16. Quick Reference Checklist](#16-quick-reference-checklist)

---

## 1. General Rules

### 1.1 Language Standard
- **MUST** use C99 standard
- **MUST NOT** use tabs; use spaces instead
- **MUST** use 4 spaces per indent level

### 1.2 Spacing Rules
- **MUST** use 1 space between keyword and opening bracket: `if (condition)`, `while (condition)`, `for (init; condition; step)`
- **MUST NOT** use space between function name and opening bracket: `sum(4, 3)` ✓, `sum (4, 3)` ✗
- **MUST** use single space before and after comparison and assignment operators: `a = 3 + 4;` ✓
- **MUST** use single space after every comma: `func_name(5, 4);` ✓

**Examples:**
```c
/* Correct spacing */
if (x > 0) {
    int32_t result = calculate(a, b, c);
    for (size_t i = 0; i < 10; ++i) {
        array[i] = i * 2;
    }
}

/* Wrong spacing */
if(x>0){                    /* Missing spaces */
    int32_t result=calculate(a,b,c);
    for(size_t i=0;i<10;++i){
        array[i]=i*2;
    }
}
```

### 1.3 Naming Conventions
- **MUST NOT** use `__` or `_` prefix for variables/functions/macros/types (reserved for C language)
- **MUST** use only lowercase characters for variables/functions/types with optional underscore `_`
- **SHOULD** use `prv_` prefix for strictly module-private (static) functions
- **SHOULD** use `libname_int_` or `libnamei_` prefix for library internal functions used across modules
- **SHOULD** use `module_get_xxx` or `module_set_xxx` naming for external functions

**Examples:**
```c
/* Correct naming */
static void prv_initialize_buffer(void);           /* Private function */
void battery_get_voltage(uint16_t* voltage);       /* Public getter */
void battery_set_threshold(uint16_t threshold);    /* Public setter */
static int32_t battery_int_calculate_soc(void);    /* Internal function */

/* Wrong naming */
static void _initialize_buffer(void);              /* Reserved prefix */
void __battery_get_voltage(void);                  /* Reserved prefix */
void BatteryGetVoltage(void);                      /* Mixed case */
void battery_get_VOLTAGE(void);                    /* Mixed case */
```

### 1.4 Braces and Blocks
- **MUST** place opening curly bracket on the same line as keyword: `for (i = 0; i < 5; ++i) { }` ✓
- **MUST** include braces for all compound statements, even single statements: `if (c) { do_a(); }` ✓
- **MUST** use single indent for each block level

---

## 2. Variables

### 2.1 Declaration Rules
- **MUST** declare all local variables of the same type on the same line: `char a, b;` ✓
- **MUST** declare local variables at the beginning of a block, before first executable statement
- **MUST** declare variables in order:
  1. Custom structures and enumerations
  2. Integer types (wider unsigned first: uint32_t, int32_t, uint16_t, int16_t, char)
  3. Single/Double floating point

### 2.2 Variable Usage
- **MUST** use `size_t` for length or size variables
- **MUST** use `const` for pointers if function should not modify pointed memory
- **MUST** use `const` for function parameters or variables that should not be modified
- **MUST** always compare pointers with `NULL`
- **MUST** always compare variables with zero unless treated as boolean: `if (length > 0)` ✓, `if (is_ok)` ✓
- **SHOULD** use pre-increment instead of post-increment: `++a` ✓ instead of `a++`
- **MUST NOT** use Variable Length Arrays (VLA); use `malloc()` and `free()` instead
- **MUST NOT** initialize global variables to default values; implement in dedicated `init` function

### 2.3 Pointer Usage
- **MUST** use `void*` when function accepts any type of pointer
- **MUST NOT** cast return value of `malloc()` or `void*` functions
- **MUST** use `const void*` if function should not modify pointed memory

### 2.4 Boolean Handling
- **MUST NOT** use `stdbool.h`; use 1 or 0 instead
- **MUST NOT** compare boolean variables with 1: `if (is_ok == 1)` ✗
- **MUST** use `!` for negative boolean check: `if (!is_ok)` ✓

---

## 3. Functions

### 3.1 Function Declaration
- **MUST** provide function prototype for all functions accessible outside module
- **MUST** use lowercase function names with optional underscores: `my_func()` ✓, `MYFunc()` ✗
- **MUST** align star with return type for pointer returns: `const char* my_func(void);` ✓
- **SHOULD** align function prototypes with similar functionality for readability

### 3.2 Function Implementation
- **MUST** include return type and optional keywords on same line as function name
- **MUST** include doxygen documentation for every function, even static ones
- **MUST** use English names/text for functions, variables, and comments

### 3.3 Function Parameters
- **MUST** use `const` for parameters that should not be modified
- **MUST** mark input parameters with `[in]` and output parameters with `[out]` in documentation

---

## 4. Structures, Enumerations, and Typedefs

### 4.1 Naming Rules
- **MUST** use lowercase names with optional underscores
- **MUST** use `_t` suffix only for typedef'd structures/enums
- **MUST NOT** use `_t` suffix for named structures without typedef

### 4.2 Structure Declaration Patterns

**Pattern 1: Named structure only (no typedef)**
```c
struct struct_name {
    char* a;
    char b;
};
```

**Pattern 2: Typedef only (anonymous struct)**
```c
typedef struct {
    char* a;
    char b;
} struct_name_t;
```

**Pattern 3: Named structure with typedef**
```c
typedef struct struct_name {    /* No _t suffix */
    char* a;
    char b;
} struct_name_t;                /* _t suffix */
```

### 4.3 Structure Initialization
- **MUST** use C99 designated initializer style: `.a = 1, .b = 2`
- **SHOULD** add trailing comma after last element for complex structures (helps clang-format)
- **MUST** declare each member on separate line

### 4.4 Enumeration Rules
- **MUST** use all uppercase for enum members: `MY_ENUM_VALUE` ✓
- **MUST** use `_t` suffix for typedef'd enums
- **MUST** include doxygen documentation

### 4.5 Function Pointer Typedefs
- **MUST** use `_fn` suffix for function pointer typedefs:
```c
typedef uint8_t (*my_func_typedef_fn)(uint8_t p1, const char* p2);
```

---

## 5. Macros and Preprocessor

### 5.1 Macro Naming and Protection
- **MUST** use all uppercase names with optional underscores: `MY_MACRO` ✓
- **MUST** parenthesize all parameters: `#define MIN(x, y) ((x) < (y) ? (x) : (y))` ✓
- **MUST** parenthesize entire macro expression: `#define SUM(x, y) ((x) + (y))` ✓
- **MUST NOT** evaluate side-effect parameters multiple times

### 5.2 Multi-Statement Macros
- **MUST** wrap multi-statement macros in `do { } while (0)`:
```c
#define SET_POINT(p, x, y) do { (p)->px = (x); (p)->py = (y); } while (0)
```

### 5.3 Common Macros
```c
#define XY_MIN(x, y)        ((x) < (y) ? (x) : (y))
#define XY_MAX(x, y)        ((x) > (y) ? (x) : (y))
#define XY_BIT(pos)         (1UL << (pos))
#define XY_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define XY_CLAMP(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
#define XY_UNUSED(x)        do { (void)(x); } while (0)
```

### 5.4 Conditional Compilation
- **MUST** use `defined()` or `!defined()` instead of `#ifdef` or `#ifndef`
- **MUST** document all `#if/#elif/#else/#endif` statements:
```c
#if defined(XYZ)
    /* Do if XYZ defined */
#else /* defined(XYZ) */
    /* Do if XYZ not defined */
#endif /* !defined(XYZ) */
```
- **MUST NOT** indent sub-statements within `#if` blocks

### 5.5 Macro vs Inline Functions
- **SHOULD** prefer `static inline` functions over macros for logic exceeding 1-2 operations
- **SHOULD** prefer `static inline` functions when type safety is needed

---

## 6. Comments

### 6.1 Comment Style
- **MUST NOT** use `//` comments
- **MUST** use `/* simple comment. */` for single-line comments
- **MUST** use `/**< member comment */` for structure member comments
- **MUST** use `space+asterisk` for every line in multi-line comments:
```c
/*
 * This is multi-line comment
 * written in 2 lines (ok)
 */
```

### 6.2 Comment Alignment
- **SHOULD** align comments at 12 indent levels (12 * 4 spaces) from line beginning
- **SHOULD** align to next available 4-space indent if statement exceeds 12 indents

---

## 7. Compound Statements

### 7.1 If/Else Statements
- **MUST** include braces for all compound statements
- **MUST** place `else` on same line as closing brace of `if`: `} else {` ✓
- **MUST** use `else if` for chained conditions

### 7.2 Do-While Statements
- **MUST** place `while` on same line as closing brace: `} while (condition);` ✓

### 7.3 Empty Loops
- **MUST** use empty single-line braces for empty loops: `while (condition) {}` ✓
- **MUST NOT** use space inside empty braces: `while (condition) { }` ✗

### 7.4 Loop Preferences
- **SHOULD** prefer `for` loops over `while` loops
- **SHOULD** prefer `while` loops over `do-while` loops
- **SHOULD** declare loop counter in `for` statement when possible: `for (size_t i = 0; i < 10; ++i)`

### 7.5 Ternary Operator
- **SHOULD** use ternary operator only for assignments or function calls
- **MUST NOT** use for function calls without assignment: `condition ? func_a() : func_b();` ✗

---

## 8. Switch Statements

### 8.1 Switch Structure
- **MUST** indent each `case` statement by single indent
- **MUST** indent statements within `case` by additional indent
- **MUST** indent `break` statement with additional indent
- **MUST** always include `default` case

### 8.2 Switch with Local Variables
- **MUST** use braces for `case` blocks requiring local variables
- **MUST** place opening brace on same line as `case`: `case 0: {` ✓
- **MUST** place `break` inside braces

```c
switch (a) {
    case 0: {
        int32_t a, b;
        a = 5;
        break;
    }
    default:
        break;
}
```

---

## 9. Documentation (Doxygen)

### 9.1 File Documentation
- **MUST** include doxygen `\file` and `\brief` at file beginning
- **MUST** include license header (single asterisk, not double)
- **MUST** leave empty line after file header

### 9.2 Function Documentation
- **MUST** include `\brief` description
- **MUST** document all parameters with `\param[in]` or `\param[out]`
- **MUST** include `\return` for non-void functions
- **SHOULD** use `\note` or `\warning` for additional information
- **MUST** use colon between parameter name and description: `\param[in] a: First number`

### 9.3 Macro Documentation
- **MUST** include `\hideinitializer` command for macro documentation

### 9.4 Structure/Enum Documentation
- **MUST** use `/**< member comment */` for member documentation
- **MUST** follow doxygen syntax for structures and enumerations

---

## 10. Header and Source Files

### 10.1 Header File Rules
- **MUST** include include guard: `#ifndef XY_FILE_H`
- **MUST** include C++ extern check:
```c
#ifdef __cplusplus
extern "C" {
#endif
```
- **MUST** include only necessary headers for compilation
- **MUST** expose only public module variables/types/functions
- **MUST** use `extern` for global module variables
- **MUST NOT** include module private declarations
- **MUST** leave empty line at file end

### 10.2 Source File Rules
- **MUST** include corresponding header file first
- **MUST NOT** include `.c` files in other `.c` files
- **MUST** leave empty line at file end

### 10.3 Include Order
1. Standard C library headers
2. Application custom headers

---

## 11. Conditional Compilation Examples

### 11.1 Multiple Defined Conditions
```c
#if defined(CREDIT)
    credit();
#elif defined(DEBIT) && defined(DEBIT_ENABLED)
    debit();
#else
    print_error();
#endif
```

### 11.2 Nested Conditional Compilation
```c
#if DLEVEL > 5
    #define SIGNAL 1
    #if STACKUSE == 1
        #define STACK 200
    #else
        #define STACK 100
    #endif
#else
    #define SIGNAL 0
    #if STACKUSE == 1
        #define STACK 100
    #else
        #define STACK 50
    #endif
#endif
```

---

## 12. Code Organization Best Practices

### 12.1 Variable Declaration Order
```c
int my_func(void) {
    /* 1. Custom structures and pointers */
    my_struct_t my;
    my_struct_ptr_t* p;

    /* 2. Integer types (wider unsigned first) */
    uint32_t a;
    int32_t b;
    uint16_t c;
    int16_t g;
    char h;

    /* 3. Floating point */
    double d;
    float f;
}
```

### 12.2 Structure Initialization
```c
typedef struct {
    int a, b;
} str_t;

str_t s = {
    .a = 1,
    .b = 2,  /* Trailing comma for complex structures */
};
```

### 12.3 Avoid Function Calls in Declarations
```c
/* Avoid */
int32_t a, b = sum(1, 2);

/* Prefer */
int32_t a, b;
b = sum(1, 2);

/* OK for single variable */
uint8_t a = 3, b = 4;
```

---

## 13. Common Pitfalls to Avoid

| Issue | Wrong | Correct |
|-------|-------|---------|
| Spacing around keywords | `if(condition)` | `if (condition)` |
| Function call spacing | `sum (4, 3)` | `sum(4, 3)` |
| Pointer alignment | `const char *ptr` | `const char* ptr` |
| Macro parentheses | `#define MIN(x,y) x<y?x:y` | `#define MIN(x,y) ((x)<(y)?(x):(y))` |
| Boolean comparison | `if (is_ok == 1)` | `if (is_ok)` |
| Variable comparison | `if (length)` | `if (length > 0)` |
| Pointer comparison | `if (ptr)` | `if (ptr != NULL)` |
| Malloc casting | `int *p = (int *)malloc(...)` | `int *p = malloc(...)` |
| VLA usage | `int arr[size]` | `int *arr = malloc(sizeof(*arr) * size)` |
| Global initialization | `static int b = 4;` | Initialize in `init()` function |
| Comment style | `// comment` | `/* comment */` |
| Typedef suffix | `struct name_t { }` | `typedef struct { } name_t;` |
| Enum members | `MY_ENUM_testA` | `MY_ENUM_TEST_A` |

---

## 14. Formatting Tools

### 14.1 Clang-Format Configuration
- Use `.clang-format` file in `docs/code_style/` directory
- Automatically formats code to match style guidelines
- Trailing commas in structures help clang-format format correctly

### 14.2 Doxygen Configuration
- Use `doxygen.txt` configuration in `docs/code_style/` directory
- Generates documentation from code comments

---

## 15. Quick Reference Checklist

- [ ] Use C99 standard
- [ ] 4 spaces per indent, no tabs
- [ ] Lowercase names with optional underscores
- [ ] No `__` or `_` prefix for user code
- [ ] Space after keywords: `if (`, `while (`
- [ ] No space after function names: `func(`
- [ ] Braces on same line as keyword
- [ ] Braces for all compound statements
- [ ] `const` for immutable pointers/parameters
- [ ] `size_t` for lengths/sizes
- [ ] `NULL` for pointer comparisons
- [ ] `> 0` or `== 0` for numeric comparisons
- [ ] `/* */` comments, no `//`
- [ ] Doxygen documentation for all functions
- [ ] `_t` suffix for typedef'd types only
- [ ] Parenthesize macro parameters and expressions
- [ ] `do { } while (0)` for multi-statement macros
- [ ] `defined()` for conditional compilation
- [ ] Document `#if/#endif` blocks
- [ ] Empty line at file end

---

## References

- **RFC2119/RFC8174**: Keyword definitions (MUST, SHOULD, MAY, etc.)
- **C99 Standard**: Language specification
- **Doxygen**: Documentation generation tool
- **Clang-Format**: Code formatting tool
- **Project Files**: `docs/code_style/xy_code_style.md`, `xy_code_style.h`, `xy_code_style.c`
