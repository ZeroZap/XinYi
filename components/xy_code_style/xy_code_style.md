#### Conventions used

The keywords *MUST*, *MUST NOT*, *REQUIRED*, *SHALL*, *SHALL NOT*, *SHOULD*, *SHOULD NOT*, *RECOMMENDED*, *NOT RECOMMENDED*, *MAY*, and *OPTIONAL* in this document are to be interpreted as described in BCP 14 [RFC2119] [RFC8174]

本文档中的关键字*MUST*、*MUST NOT*、*REQUIRED*、*SHALL*、*SHALL NOT*、*SHOULD*、*SHOULD NOT*、*RECOMMENDED*、*NOT RECOMMENDED*、*MAY*和 *OPTIONAL*应按照 BCP 14 [RFC2119] [RFC8174] 中的描述进行解释

### General rules 一般规则



#### Use C99 standard 使用 `C99` 标准



#### Do not use tabs, use space instead 不要使用制表符，而使用空给



#### Use `4` space per indent level



#### Use `1` space between keyword and opening bracket 在关键字和左括号之间使用空格



```c
/* OK */
if (condition)
while (condition)
for (init; condition; step)
do {} while (condition)

/* Wrong */
if(condition)
while(condition)
for(init;condition;step)
do {} while(condition)
```



#### Do not uese space between function name and opening bracket 函数名称和左括号之间不要使用空格

```c
int32_t a = sum(4, 3);              /* OK */
int32_t a = sum (4, 3);             /* Wrong */
```



#### Never use `__` or `_` prefix for variables/functions/macro/types. 切勿在变量/函数/宏/类型前使用`__`or`_`前缀

`__` or `_`  is reserved for C language itslef.

- Prefer `prv_` name prefix for strictly module-private (static) functions
- Prefer `libname_int_` or `libnamei_` prefix for library internal functions, that should not be used by the user application while they MUST be used across different library internal modules



#### Use only lowercase characters for variables/functions/types with optional underscore `_` char   变量/函数/类型仅使用小写字符，并带有可选下划线_字符



#### Opening curly bracket is always at the same line as keyword (`for`, `while`, `do`, `switch`, `if`, ...) 左花括号始终与关键字位于同一行

``` c
size_t i;
for (i = 0; i < 5; ++i) {           /* OK */
}
for (i = 0; i < 5; ++i){            /* Wrong */
}
for (i = 0; i < 5; ++i)             /* Wrong */
{
}
```



#### Use single space before and after comparison and assignment operators



``` c
int32_t a;
a = 3 + 4;              /* OK */
for (a = 0; a < 5; ++a) /* OK */
a=3+4;                  /* Wrong */
a = 3+4;                /* Wrong */
for (a=0;a<5;++a)       /* Wrong */
```

#### Use single space before and after comparison and assignment operators 比较运算符和赋值运算符前后使用单个空格

```c
int32_t a;
a = 3 + 4;              /* OK */
for (a = 0; a < 5; ++a) /* OK */
a=3+4;                  /* Wrong */
a = 3+4;                /* Wrong */
for (a=0;a<5;++a)       /* Wrong */
```


#### Use single space after every comma 每个逗号后使用一个空格



``` c
func_name(5, 4);        /* OK */
func_name(4,3);         /* Wrong */
```



#### Do not initialize `global` variables to any default value (or `NULL`), implement it in the dedicated `init` function (if REQUIRED). 不要将global变量初始化为任何默认值（或NULL），在专用init函数中实现它（如果需要）。



In embedded systems, the startup process typically involves a startup script (or startup code) initializing the .data segment (initialized global and static variables) and the .bss segment (uninitialized global and static variables, typically initialized to 0). However, embedded systems may include custom memory segments (for example, placing variables in specific RAM areas such as fast RAM or non-volatile RAM). These custom segments may not be automatically initialized by standard startup scripts. Therefore, if initial values are given when declaring global variables, these initial values may not be correctly set to the memory locations where the variables reside, as the startup script may not process the custom segments.



``` c
static int32_t a;       /* OK */
static int32_t b = 4;   /* Wrong - this value may not be set at zero
                            if linker script&startup files are not properly handled */

void
my_module_init(void) {
    a = 0;
    b = 4;
}
```



#### Declare all local variables of the same type in the same line  在同一行中声明同一类型的所有局部变量

```c
void my_func(void) {
    /* 1 */
    char a;             /* OK */

    /* 2 */
    char a, b;          /* OK */

    /* 3 */
    char a;
    char b;             /* Wrong, variable with char type already exists */
}
```

- Declare local variables in order
  1. Custom structures and enumerations
  2. Integer types, wider unsigned type first
  3. Single/Double floating point

  ``` c
  int my_func(void) {
      /* 1 */
      my_struct_t my;     /* First custom structures */
      my_struct_ptr_t* p; /* Pointers too */
  
      /* 2 */
      uint32_t a;
      int32_t b;
      uint16_t c;
      int16_t g;
      char h;
      /* ... */
  
      /* 3 */
      double d;
      float f;
  }
  ```



#### 始终在块的开头、第一个可执行语句之前声明局部变量



#### 始终在结构（或其子元素）初始化的最后一个元素中添加尾随逗号（这有助于 clang-format 正确格式化结构）。除非结构非常简单且简短



```c
typedef struct {
    int a, b;
} str_t;

str_t s = {
    .a = 1,
    .b = 2,   /* Comma here */
}

/* Examples of "complex" structure, with or with missing several trailing commas, after clang-format runs the formatting */
static const my_struct_t my_var_1 = {
    .type = TYPE1,
    .type_data =
        {
            .type1 =
                {
                    .par1 = 0,
                    .par2 = 1, /* Trailing comma here */
                }, /* Trailing comma here */
        },  /* Trailing comma here */
};

static const my_struct_t my_var_2 = {.type = TYPE2,
                                     .type_data = {
                                         .type2 =
                                             {
                                                 .par1 = 0,
                                                 .par2 = 1,
                                             },
                                     }};    /* Missing comma here */
static const my_struct_t my_var_3 = {.type = TYPE3,
                                     .type_data = {.type3 = {
                                                       .par1 = 0,
                                                       .par2 = 1,
                                                   }}}; /* Missing 2 commas here */

/* No trailing commas - good only for small and simple structures */
static const my_struct_t my_var_4 = {.type = TYPE4, .type_data = {.type4 = {.par1 = 0, .par2 = 1}}};
```



#### for 循环中声明计数器变量

``` c
/* OK */
for (size_t i = 0; i < 10; ++i)

/* OK, if you need counter variable later */
size_t i;
for (i = 0; i < 10; ++i) {
    if (...) {
        break;
    }
}
if (i == 10) {

}

/* Wrong */
size_t i;
for (i = 0; i < 10; ++i) ...
```



#### 避免在声明中使用函数调用进行变量赋值，单个变量除外

``` c
void a(void) {
    /* Avoid function calls when declaring variable */
    int32_t a, b = sum(1, 2);

    /* Use this */
    int32_t a, b;
    b = sum(1, 2);

    /* This is ok */
    uint8_t a = 3, b = 4;
}
```



#### 不要使用库stdbool.h。分别使用1或0或truefalse



#### 切勿与 进行比较true，例如if (check_func() == 1)使用if (check_func()) { ... }



#### 始终将指针与NULL值进行比较



#### 可能使用预增（减）量而不是后增（减）量

``` c
int32_t a = 0;
...

a++;            /*  */
++a;            /* Preferred */

for (size_t j = 0; j < 10; ++j) {}  /* OK */
```



#### 始终用于`size_t`长度或大小变量



#### const如果函数不应该修改指向的内存，则始终使用指针pointer



#### const如果不应修改，则始终将其用于函数参数或变量

 ```c

/* When d could be modified, data pointed to by d could not be modified */
void
my_func(const void* d) {

}

/* When d and data pointed to by d both could not be modified */
void
my_func(const void* const d) {

}

/* Not REQUIRED, it is advised */
void
my_func(const size_t len) {

}

/* When d should not be modified inside function, only data pointed to by d could be modified */
void
my_func(void* const d) {

}
 ```



#### 当函数可以接受任何类型的指针时，始终使用void *，不要使用uint8_t * 而是函数在实现时必须注意正确的转换

``` c
/*
 * To send data, function should not modify memory pointed to by `data` variable
 * thus `const` keyword is important
 *
 * To send generic data (or to write them to file)
 * any type may be passed for data,
 * thus use `void *`
 */
/* OK example */
void
send_data(const void* data, size_t len) { /* OK */
    /* Do not cast `void *` or `const void *` */
    const uint8_t* d = data;/* Function handles proper type for internal usage */
}

void
send_data(const void* data, int len) {    /* Wrong, not not use int */
}
```



#### 始终使用带有sizeof运算符的括号



#### 切勿使用可变长度数组malloc(VLA)。请使用标准 C和函数的动态内存分配free，或者如果库/项目提供了自定义内存分配，请使用其实现。

- 看看[LwMEM](https://github.com/MaJerle/lwmem)，自定义内存管理库



``` c
/* OK */
#include <stdlib.h>
void
my_func(size_t size) {
    int32_t* arr;
    arr = malloc(sizeof(*arr) * n); /* OK, Allocate memory */
    arr = malloc(sizeof *arr * n);  /* Wrong, brackets for sizeof operator are missing */
    if (arr == NULL) {
        /* FAIL, no memory */
    }

    free(arr);  /* Free memory after usage */
}

/* Wrong */
void
my_func(size_t size) {
    int32_t arr[size];  /* Wrong, do not use VLA */
}
```



#### 总是将变量与零进行比较，除非将其视为boolean类型


### Macros 宏规范与示例

良好的宏设计可以提高可读性并避免副作用。以下原则和示例与`xy_code_style.h`中的实现保持一致。

#### 通用规则 General
1. 参数与最终展开结果必须使用括号保护。Always parenthesize parameters and full expression.
2. 多语句宏必须使用`do { } while (0)`包装，保证其在任何语法环境下都是单一语句。Wrap multi-statement macros in `do { } while (0)`.
3. 宏名使用全部大写并可含下划线。Names MUST be uppercase with optional underscores.
4. 不产生副作用 (如 ++/--) 的参数在宏内部不要重复求值。Never evaluate side-effect parameters multiple times.
5. 对值范围进行限制时优先使用 CLAMP 宏而不是手写嵌套 if。Prefer a CLAMP macro over ad-hoc nested if logic.

#### 简单数值类宏 Simple value macros

```c
/* OK */
#define XY_MIN(x, y)            ((x) < (y) ? (x) : (y))
#define XY_MAX(x, y)            ((x) > (y) ? (x) : (y))

/* Wrong: missing parens around parameters and whole expression */
#define MY_MIN(x, y) x < y ? x : y
```

#### 位操作 Bit helpers

```c
/* OK: 单一位掩码 bit mask */
#define XY_BIT(pos)             (1UL << (pos))

/* 使用示例 */
uint32_t mask = XY_BIT(5); /* mask == 0x20 */
```

#### 数组长度 Array size

```c
/* OK */
#define XY_ARRAY_SIZE(arr)      (sizeof(arr) / sizeof((arr)[0]))

static const int32_t values[] = {1, 3, 5};
size_t count = XY_ARRAY_SIZE(values); /* count == 3 */

/* Wrong: 缺少括号, 运算优先级风险 */
#define BAD_ARR_SZ(a) sizeof a / sizeof a[0]
```

#### 值限制 Clamping

```c
#define XY_CLAMP(v, min, max)   (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

int32_t speed = XY_CLAMP(input_speed, 0, 100); /* 保证范围在0..100 */
```

#### 复合语句宏 Multi-statement

```c
#define XY_SET_POINT(p, x, y)   do { (p)->px = (x); (p)->py = (y); } while (0)

xy_point_t pt;
XY_SET_POINT(&pt, 10, 20);
```

#### 未使用参数标记 Unused variable marker

```c
#define XY_UNUSED(x)            do { (void)(x); } while (0)

void handler(int code) {
    XY_UNUSED(code); /* 消除未使用警告 */
}
```

#### 避免副作用 Avoid side effects

```c
int i = 0;
/* Wrong: MIN(i++, 5) 会对 i 做两次自增 */
int wrong = XY_MIN(i++, 5); /* i 可能增加两次 */

/* Better: 对含副作用的表达式先存储到临时变量 */
int tmp = i++;
int ok = XY_MIN(tmp, 5);
```

#### 宏 vs 内联函数 Macro vs inline function

当逻辑超过 1~2 个操作或需要类型安全时，优先考虑使用`static inline`函数而不是宏。

```c
static inline int32_t clamp_i32(int32_t v, int32_t min, int32_t max) {
    return v < min ? min : (v > max ? max : v);
}
```

#### 与示例代码对应 Mapping to demo code

| 宏 | 位置 | 示例函数 | 用途 |
|----|------|----------|------|
| XY_MIN / XY_MAX | `xy_code_style.h` | `demo_get_min` | 数值比较 |
| XY_BIT | `xy_code_style.h` | `demo_build_mask` | 位掩码构造 |
| XY_ARRAY_SIZE | `xy_code_style.h` | `demo_count_values` | 静态数组长度 |
| XY_CLAMP | `xy_code_style.h` | `demo_clamp_value` | 限定取值范围 |
| XY_SET_POINT | `xy_code_style.h` | `demo_point_init` | 结构体成员写入 |
| XY_UNUSED | `xy_code_style.h` | `demo_mark_unused` | 消除未使用警告 |

> NOTE: 对于复杂数据结构初始化、资源管理（如分配/释放），避免使用宏，使用常规函数实现更安全。



#### 总是将变量与零进行比较，除非将其视为boolean类型



``` c
size_t length = 5;  /* Counter variable */
uint8_t is_ok = 0;  /* Boolean-treated variable */
if (length)         /* Wrong, length is not treated as boolean */
if (length > 0)     /* OK, length is treated as counter variable containing multi values, not only 0 or 1 */
if (length == 0)    /* OK, length is treated as counter variable containing multi values, not only 0 or 1 */

if (is_ok)          /* OK, variable is treated as boolean */
if (!is_ok)         /* OK, -||- */
if (is_ok == 1)     /* Wrong, never compare boolean variable against 1! */
if (is_ok == 0)     /* Wrong, use ! for negative check */
```



#### 始终在头文件中C++包含检查关键字extern



#### 始终在头文件中C++包含检查关键字extern



#### 每个函数都必须包含启用 doxygen 的注释，即使函数是static



#### 对函数、变量、注释使用英文名称/文本



#### 对变量使用小写字符



#### 如果变量包含多个名称，请使用下划线force_redraw，例如。不要使用forceRedraw





#### 切勿强制转换返回的函数void *



``` c
/* OK */
int *ptr = malloc(sizeof(int) * 10);
char *str = malloc(100);

/* Wrong */
int *ptr = (int *)malloc(sizeof(int) * 10);
char *str = (char *)malloc(100);

typedef struct {
    void *data;
    size_t element_size;
    size_t capacity;
} generic_container_t;


generic_container_t* create_container(size_t element_size, size_t capacity) {
    generic_container_t *container = malloc(sizeof(generic_container_t));
    if (!container) return NULL;

    container->data = malloc(element_size * capacity);
    container->element_size = element_size;
    container->capacity = capacity;

    return container;
}


void* get_element(generic_container_t *container, size_t index) {
    if (!container || index >= container->capacity)
        return NULL;

    return (char*)container->data + index * container->element_size;
}


void example_usage(void) {

    generic_container_t *int_container = create_container(sizeof(int), 100);

    /* ✅ */
    int *first_element = get_element(int_container, 0);
    if (first_element) {
        *first_element = 42;
    }

    generic_container_t *double_container = create_container(sizeof(double), 50);
    double *elem = get_element(double_container, 0);
    if (elem) {
        *elem = 3.14;
    }
}
```



### 注释

#### 不允许以 // 开头的注释



#### 使用 /* simple comment. */  单行注释

``` c
#ifndef XX
int a; /* it's a. */
#endif  /* XX */
/* end fo doxygen */
```



#### 使用 /**< member comment */  做成员单行注释

``` c
struct st {
    int a; /**< this member a. */
};
```



#### 对于多行注释，请使用`space+asterisk`for every line

```c
/*
 * This is multi-line comments,
 * written in 2 lines (ok)
 */

/**
 * Wrong, use double-asterisk only for doxygen documentation
 */

/*
* Single line comment without space before asterisk (wrong)
*/

/*
 * Single line comment in multi-line configuration (wrong)
 */

/* Single line comment (ok) */
```





注释时使用12缩进（12 * 4空格）偏移。如果语句大于12缩进，则将注释4-spaces对齐到下一个可用的缩进（如下例所示）

``` c
void
my_func(void) {
    char a, b;

    a = call_func_returning_char_a(a);          /* This is comment with 12*4 spaces indent from beginning of line */
    b = call_func_returning_char_a_but_func_name_is_very_long(a);   /* This is comment, aligned to 4-spaces indent */
}
```



## 函数

#### 每个可以从模块外部访问的函数都必须包含函数原型（或声明）



#### 函数名称必须小写，可选用下划线_分隔



``` c
/* OK */
void my_func(void);
void myfunc(void);

/* Wrong */
void MYFunc(void);
void myFunc();
```



#### 当函数返回指针时，将星号与返回类型对齐



```c
/* OK */
const char* my_func(void);
my_struct_t* my_func(int32_t a, int32_t b);

/* Wrong */
const char *my_func(void);
my_struct_t * my_func(void);
```



#### 对齐所有函数原型（具有相同/相似的功能）以提高可读性



``` c
/* OK, function names aligned */
void        set(int32_t a);
my_type_t   get(void);
my_ptr_t*   get_ptr(void);

/* Wrong */
void set(int32_t a);
const char * get(void);
```



#### 函数实现必须包含返回类型和可选的其他关键字



``` c
/* OK */
int32_t foo(void) {
    return 0;
}

/* OK */
static const char* get_string(void) {
    return "Hello world!\r\n";
}

/* Wrong */
int32_t foo(void) {
    return 0;
}
```



### 变量

#### 变量名全部小写，并带有可选的下划线_字符



``` c
/* OK */
int32_t a;
int32_t my_var;
int32_t myvar;

/* Wrong */
int32_t A;
int32_t myVar;
int32_t MYVar;
```



#### 将局部变量分组在一起`type`

####

``` c
void foo(void) {
    int32_t a, b;   /* OK */
    char a;
    char b;         /* Wrong, char type already exists */
}
```



#### 不要在第一个可执行语句后声明变量

``` c
void foo(void) {
    int32_t a;
    a = bar();
    int32_t b;      /* Wrong, there is already executable statement */
}
```



#### 您可以在下一个缩进级别内声明新变量



``` c
int32_t a, b;
a = foo();
if (a) {
    int32_t c, d;   /* OK, c and d are in if-statement scope */
    c = foo();
    int32_t e;      /* Wrong, there was already executable statement inside block */
}
```



#### 声明多个指针变量时，可以用与变量名对齐的星号来声明它们



``` c
/* OK */
char *p, *n;
```



## ### 属性函数的声明

**内部属性函数**

内部属性函数可以 以 set_xx，get_xx 开头，

**外部函数**

外部函数需要加上模块的开头，如  module_get_xxx





### 结构、枚举、typedef



结构或枚举名称必须小写，_单词之间可选下划线字符

#### 结构或枚举可能包含typedef关键字

#### 所有结构成员必须小写

#### 所有枚举成员都应该小写

#### 结构/枚举必须遵循 doxygen 文档语法

#### 结构体声明方式

1. *当仅使用名称*声明结构时，其名称后*不得*包含后缀。`_t`

```
struct struct_name {
    char* a;
    char b;
};
```



1. *当仅使用 typedef*声明结构时，其名称后*必须*包含后缀。`_t`

```
typedef struct {
    char* a;
    char b;
} struct_name_t;
```



1. *当使用名称和 typedef*声明结构时，它*不能*包含`_t`基本名称，并且*必须*在其名称后包含`_t`typedef 部分的后缀。

```
typedef struct struct_name {    /* No _t */
    char* a;
    char b;
    char c;
} struct_name_t;    /* _t */
```



错误声明示例及其建议的修正

```
/* a and b MUST be separated to 2 lines */
/* Name of structure with typedef MUST include _t suffix */
typedef struct {
    int32_t a, b;
} a;

/* Corrected version */
typedef struct {
    int32_t a;
    int32_t b;
} a_t;

/* Wrong name, it MUST not include _t suffix */
struct name_t {
    int32_t a;
    int32_t b;
};

/* Wrong parameters, MUST be all uppercase */
typedef enum {
    MY_ENUM_TESTA,
    my_enum_testb,
} my_enum_t;
```



#### 在声明时初始化结构时，使用C99初始化样式



``` c
/* OK */
a_t a = {
    .a = 4,
    .b = 5,
};

/* Wrong */
a_t a = {1, 2};
```



#### 当为函数句柄引入新的 typedef 时，使用_fn后缀

``` c
/* Function accepts 2 parameters and returns uint8_t */
/* Name of typedef has `_fn` suffix */
typedef uint8_t (*my_func_typedef_fn)(uint8_t p1, const char* p2);
```



## 复合语句

#### 每个复合语句必须包含左花括号和右花括号，即使它只包含1嵌套语句

#### 每个复合语句必须包含单个缩进；嵌套语句时，包含1每个嵌套的缩进大小

``` c
/* OK */
if (c) {
    do_a();
} else {
    do_b();
}

/* Wrong */
if (c)
    do_a();
else
    do_b();

/* Wrong */
if (c) do_a();
else do_b();
```



#### 如果是if或if-else-if语句，else必须与第一个语句的右括号位于同一行



``` c
/* OK */
if (a) {

} else if (b) {

} else {

}

/* Wrong */
if (a) {

}
else {

}

/* Wrong */
if (a) {

}
else
{

}
```



#### 对于do-while声明，部分必须与部分while的右括号位于同一行do



``` c
/* OK */
do {
    int32_t a;
    a = do_a();
    do_b(a);
} while (check());

/* Wrong */
do
{
/* ... */
} while (check());

/* Wrong */
do {
/* ... */
}
while (check());
```



#### 每个左括号都必须缩进



``` c
if (a) {
    do_a();
} else {
    do_b();
    if (c) {
        do_c();
    }
}
```



#### 复合语句必须包含花括号，即使是单条语句。以下示例展示了一些不良做法

``` c
if (a) do_b();
else do_c();

if (a) do_a(); else do_b();
```



#### 空的while,do-while或for循环必须包含括号



``` c
/* OK */
while (is_register_bit_set()) {}

/* Wrong */
while (is_register_bit_set());
while (is_register_bit_set()) { }
while (is_register_bit_set()) {
}
```



#### 如果while(或for, do-while, 等) 为空（嵌入式编程中可能出现这种情况），则使用空的单行括号



```c
/* Wait for bit to be set in embedded hardware unit */
volatile uint32_t* addr = HW_PERIPH_REGISTER_ADDR;

/* Wait bit 13 to be ready */
while (*addr & (1 << 13)) {}        /* OK, empty loop contains no spaces inside curly brackets */
while (*addr & (1 << 13)) { }       /* Wrong */
while (*addr & (1 << 13)) {         /* Wrong */

}
while (*addr & (1 << 13));          /* Wrong, curly brackets are missing. Can lead to compiler warnings or unintentional bugs */
```



#### 总是倾向于按以下顺序使用循环：for，，do-whilewhile



#### 如果可能的话，避免在循环块内增加变量



``` c
/* Not recommended */
int32_t a = 0;
while (a < 10) {
    .
    ..
    ...
    ++a;
}

/* Better */
for (size_t a = 0; a < 10; ++a) {

}

/* Better, if inc may not happen in every cycle */
for (size_t a = 0; a < 10; ) {
    if (...) {
        ++a;
    }
}
```



#### 内联if语句只能用于赋值或函数调用操作



``` c
/* OK */
int a = condition ? if_yes : if_no; /* Assignment */
func_call(condition ? if_yes : if_no); /* Function call */
switch (condition ? if_yes : if_no) {...}   /* OK */

/* Wrong, this code is not well maintenable */
condition ? call_to_function_a() : call_to_function_b();

/* Rework to have better program flow */
if (condition) {
    call_to_function_a();
} else {
    call_to_function_b();
}
```







### switch 语句



#### 为每个语句添加单个缩进case



#### 在每个或语句中使用额外的单缩进语句breakcasedefault



``` c
/* OK, every case has single indent */
/* OK, every break has additional indent */
switch (check()) {
    case 0:
        do_a();
        break;
    case 1:
        do_b();
        break;
    default:
        break;
}

/* Wrong, case indent missing */
switch (check()) {
case 0:
    do_a();
    break;
case 1:
    do_b();
    break;
default:
    break;
}

/* Wrong */
switch (check()) {
    case 0:
        do_a();
    break;      /* Wrong, break MUST have indent as it is under case */
    case 1:
    do_b();     /* Wrong, indent under case is missing */
    break;
    default:
        break;
}
```



#### 始终包含default语句



```c
/* OK */
switch (var) {
    case 0:
        do_job();
        break;
    default:
        break;
}

/* Wrong, default is missing */
switch (var) {
    case 0:
        do_job();
        break;
}
```



#### 如果局部变量是必需的，请使用花括号并将break语句放在里面

- `case`将左花括号放在与语句相同的行

```c
switch (a) {
    /* OK */
    case 0: {
        int32_t a, b;
        char c;
        a = 5;
        /* ... */
        break;
    }

    /* Wrong */
    case 1:
    {
        int32_t a;
        break;
    }

    /* Wrong, break shall be inside */
    case 2: {
        int32_t a;
    }
    break;
}
```







### 宏和预处理器指令

#### 始终使用宏而不是文字常量，尤其是对于数字



#### 所有宏必须全部大写，并带有可选的下划线_字符，除非它们明确标记为函数，将来可能会被常规函数语法取代



#### 始终用括号保护输入参数

``` c
/* OK */
#define MIN(x, y)           ((x) < (y) ? (x) : (y))

/* Wrong */
#define MIN(x, y)           x < y ? x : y
```



始终用括号保护最终的宏评估



``` c
/* Wrong */
#define MIN(x, y)           (x) < (y) ? (x) : (y)
#define SUM(x, y)           (x) + (y)

/* Imagine result of this equation using wrong SUM implementation */
int32_t x = 5 * SUM(3, 4);  /* Expected result is 5 * 7 = 35 */
int32_t x = 5 * (3) + (4);  /* It is evaluated to this, final result = 19 which is not what we expect */

/* Correct implementation */
#define MIN(x, y)           ((x) < (y) ? (x) : (y))
#define SUM(x, y)           ((x) + (y))
```



#### 当宏使用多个语句时，保护这些usingdo {} while (0)语句

```c
typedef struct {
    int32_t px, py;
} point_t;
point_t p;                  /* Define new point */

/* Wrong implementation */

/* Define macro to set point */
#define SET_POINT(p, x, y)  (p)->px = (x); (p)->py = (y)    /* 2 statements. Last one should not implement semicolon */

SET_POINT(&p, 3, 4);        /* Set point to position 3, 4. This evaluates to... */
(&p)->px = (3); (&p)->py = (4); /* ... to this. In this example this is not a problem. */

/* Consider this ugly code, however it is valid by C standard (not recommended) */
if (a)                      /* If a is true */
    if (b)                  /* If b is true */
        SET_POINT(&p, 3, 4);/* Set point to x = 3, y = 4 */
    else
        SET_POINT(&p, 5, 6);/* Set point to x = 5, y = 6 */

/* Evaluates to code below. Do you see the problem? */
if (a)
    if (b)
        (&p)->px = (3); (&p)->py = (4);
    else
        (&p)->px = (5); (&p)->py = (6);

/* Or if we rewrite it a little */
if (a)
    if (b)
        (&p)->px = (3);
        (&p)->py = (4);
    else
        (&p)->px = (5);
        (&p)->py = (6);

/*
 * Ask yourself a question: To which `if` statement does the `else` keyword belong?
 *
 * Based on first part of code, answer is straight-forward. To inner `if` statement when we check `b` condition
 * Actual answer: Compilation error as `else` belongs nowhere
 */

/* Better and correct implementation of macro */
#define SET_POINT(p, x, y)  do { (p)->px = (x); (p)->py = (y); } while (0)    /* 2 statements. No semicolon after while loop */
/* Or even better */
#define SET_POINT(p, x, y)  do {    \   /* Backslash indicates statement continues in new line */
    (p)->px = (x);                  \
    (p)->py = (y);                  \
} while (0)                             /* 2 statements. No semicolon after while loop */

/* Now original code evaluates to */
if (a)
    if (b)
        do { (&p)->px = (3); (&p)->py = (4); } while (0);
    else
        do { (&p)->px = (5); (&p)->py = (6); } while (0);

/* Every part of `if` or `else` contains only `1` inner statement (do-while), hence this is valid evaluation */

/* To make code perfect, use brackets for every if-ifelse-else statements */
if (a) {                    /* If a is true */
    if (b) {                /* If b is true */
        SET_POINT(&p, 3, 4);/* Set point to x = 3, y = 4 */
    } else {
        SET_POINT(&p, 5, 6);/* Set point to x = 5, y = 6 */
    }
}
```



#### 避免使用#ifdefor #ifndef。请改用defined()or!defined()



#### 始终记录if/elif/else/endif声明



``` c
/* OK */
#if defined(XYZ)
/* Do if XYZ defined */
#else /* defined(XYZ) */
/* Do if XYZ not defined */
#endif /* !defined(XYZ) */

/* Wrong */
#if defined(XYZ)
/* Do if XYZ defined */
#else
/* Do if XYZ not defined */
#endif
```



#### 不要在#if语句内缩进子语句



``` c
/* OK */
#if defined(XYZ)
#if defined(ABC)
/* do when ABC defined */
#endif /* defined(ABC) */
#else /* defined(XYZ) */
/* Do when XYZ not defined */
#endif /* !defined(XYZ) */

/* Wrong */
#if defined(XYZ)
    #if defined(ABC)
        /* do when ABC defined */
    #endif /* defined(ABC) */
#else /* defined(XYZ) */
    /* Do when XYZ not defined */
#endif /* !defined(XYZ) */
```



### 文档

文档代码允许 doxygen 解析和生成 html/pdf/latex 输出，因此在项目早期阶段正确执行此操作非常重要。



#### 使用 doxygen 支持的文档样式variables，functions以及



#### structures/enumerations



#### 始终使用5x4空格（5制表符）偏移文本行的开头



#### 函数文档必须写在函数实现中（通常是源文件）

#### 函数必须包含brief所有参数的文档

#### 每个参数必须分别注明是输入in还是out输出

#### 如果函数返回值，则必须包含return参数。这不适用于void函数

#### 函数可以包含其他 doxygen 关键字，例如note或warning

#### :参数名称和其描述之间使用冒号



``` c
/**
 * \brief           Sum `2` numbers
 * \param[in]       a: First number
 * \param[in]       b: Second number
 * \return          Sum of input values
 */
int32_t
sum(int32_t a, int32_t b) {
    return a + b;
}

/**
 * \brief           Sum `2` numbers and write it to pointer
 * \note            This function does not return value, it stores it to pointer instead
 * \param[in]       a: First number
 * \param[in]       b: Second number
 * \param[out]      result: Output variable used to save result
 */
void
void_sum(int32_t a, int32_t b, int32_t* result) {
    *result = a + b;
}
```



#### 如果函数返回枚举成员，则使用ref关键字指定哪一个



``` c
/**
 * \brief           My enumeration
 */
typedef enum {
    MY_ERR,                                     /*!< Error value */
    MY_OK                                       /*!< OK value */
} my_enum_t;

/**
 * \brief           Check some value
 * \return          \ref MY_OK on success, member of \ref my_enum_t otherwise
 */
my_enum_t
check_value(void) {
    return MY_OK;
}
```



#### 使用符号 (`NULL` => NULL) 表示常量或数字



``` c
/**
 * \brief           Get data from input array
 * \param[in]       in: Input data
 * \return          Pointer to output data on success, `NULL` otherwise
 */
const void *
get_data(const void* in) {
    return in;
}
```

#### 宏的文档必须包含hideinitializerdoxygen 命令

``` c
/**
 * \brief           Get minimal value between `x` and `y`
 * \param[in]       x: First value
 * \param[in]       y: Second value
 * \return          Minimal value between `x` and `y`
 * \hideinitializer
 */
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
```





### 头文件和源文件



#### 在文件末尾留一个空行



#### 每个文件必须包含 doxygen 注释file和brief描述，后跟空行（使用 doxygen 时）



``` c
/**
 * \file            template.h
 * \brief           Template include file
 */
                    /* Here is empty line */
```



#### 每个文件（标题或源）必须包含许可证（开头注释包含单个星号，因为 doxygen 必须忽略它）



#### 使用与项目/库相同的许可证



``` c
/**
 * \file            template.h
 * \brief           Template include file
 */

/*
 * Copyright (c) year FirstName LASTNAME
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of library_name.
 *
 * Author:          FirstName LASTNAME <optional_email@example.com>
 */
```



#### 头文件必须包含 guard#ifndef



#### 头文件必须包含C++检查



#### 包含外部头文件之外的C++检查



#### 首先在 STL C 文件中包括外部头文件，然后是应用程序自定义文件



#### 头文件必须仅包含每个其他头文件才能正确编译，但不能包含更多（如果需要，.c 应该包含其余部分）



#### 头文件必须仅公开模块公共变量/类型/函数



#### 在头文件中用于extern全局模块变量，稍后在源文件中定义它们



#### 切勿将.c文件包含到另一个.c文件中



#### .c文件应首先包含相应的.h文件，然后再包含其他文件，除非另有明确必要



#### 不要在头文件中包含模块私有声明



#### 头文件示例（为了举例说明，没有许可证）



#### 综合示例



