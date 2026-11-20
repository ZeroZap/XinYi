# XinYi C 代码风格规范

更新日期：2026-11-20

本文适用于嵌入式 C 代码及配套主机程序，可独立阅读和使用。每条规则均提供「正确」「不正确」
和「可选正确」示例。「不正确」可能表示违反风格，也可能表示存在错误，具体原因由说明指出；
「可选正确」标明适用条件，不表示可以任意混用风格。

示例直接展示代码或设置，不引用文件。代码块是声明、语句或实现片段，局部语句位于函数体内；
`size_t`、`uint8_t`、`uint32_t`、`bool`、`NULL` 等按 C99 标准定义理解。
各组独立阅读，同组内的对照写法用于替换，不要求同时放入一个程序。
示例聚焦当前规则，可以省略无关的声明和文档；真实交付代码仍需满足全部适用要求。

## 0. 约束词与适用范围

| 用词 | 含义 | 说明示例 |
| --- | --- | --- |
| **必须**（MUST／REQUIRED） | 适用范围内必须满足的要求 | 函数定义的左花括号必须单独成行 |
| **应该**（SHOULD／RECOMMENDED） | 默认采用的建议；存在明确理由时可以采用替代方案 | 参数应该不超过 5 个；配置对象可以合并相关参数 |
| **禁止**（MUST NOT） | 适用范围内不允许的行为 | 禁止在释放对象后继续访问该对象 |
| **可以**（MAY／OPTIONAL） | 明确允许的选项 | 枚举成员可以整组使用小写或全大写 |

「推荐」「建议」与「应该」同义。偏离「应该」时说明适用理由即可，不因此自动产生审批要求。
「可选正确」必须满足适用的「必须」和「禁止」；仅偏离建议的反例表示可维护性问题，
不等同于编译错误或安全缺陷。单独标为不正确的示例不是允许在交付代码中照用的选项。

本文涵盖代码风格、接口、基础安全与验证原则；不规定分支管理、推送策略、目录布局或特定平台部署流程。
第三方库、官方／权威 SDK 和生成代码必须保留上游风格，自有适配代码采用本文约定。

## 1. 语言与文本

### 1.1 使用 C99，隔离扩展语法

**必须**使用 C99。指定初始化器和循环内声明可以使用；编译器扩展仅用于有明确必要的适配层。

**正确：** 使用 C99 指定初始化器。

```c
struct point {
    int x;
    int y;
};

struct point origin = {
    .x = 0,
    .y = 0,
};
```

**不正确：** C++ 类型推导和 GNU `typeof` 不能作为通用 C99 写法。

```c
auto count = 3;
typeof(count) total = 0;
```

**可选正确：** 简单数组使用普通初始化。

```c
int values[] = {1, 2, 3};
```

### 1.2 文本使用 UTF-8，LF 与 CRLF 均可

**必须**使用 UTF-8 编码，允许 LF 或 CRLF 换行，同一文本内保持一致，不强制转换已有换行风格。
删除行尾空白，最后一行以所选换行方式结束。
以下用转义记号展示实际字符，`\t` 表示 Tab，`␠` 表示空格。

**正确：**

```text
编码：UTF-8
内容：int count = 0;\n
```

**不正确：** 同一文本混用 LF 与 CRLF、行尾有空白或缺少末尾换行。

```text
int count = 0;␠␠\n
int ready = 1;\r\n
int total = 1;<没有末尾换行>
```

**可选正确：** 全部使用 CRLF；纯 ASCII 内容也是合法 UTF-8。

```text
编码：UTF-8
内容：int count = 0;\r\nint total = 1;\r\n
```

## 2. 命名

### 2.1 普通标识符使用英文小写加下划线

变量、参数、函数、成员**应该**使用有意义的英文小写名称；**禁止**新标识符以 `_` 或 `__` 开头；
避免含糊缩写，允许领域通用缩写，不以固定字符数截断名称。

**正确：**

```c
int buffer_size = 64;
int timeout_ms = 100;
int uart_port = 1;
```

**不正确：** 混用驼峰、保留前缀和含义不清的缩写。

```c
int bufferSize = 64;
int __timeout = 100;
int bs = 64;
```

**可选正确：** 小范围循环中可以使用约定俗成的索引名。

```c
for (size_t i = 0; i < 4; ++i) {
    values[i] = 0;
}
```

### 2.2 公共接口体现模块与动作

框架公共接口**必须**使用 `xy_<模块>_<动作>`；HAL、OSAL 接口分别使用 `hal_`、`osal_` 前缀。
获取和设置接口使用 `get`、`set`，不为统一命名破坏已有公开 API 或 ABI。

**正确：**

```c
int xy_sensor_init(void);
int xy_sensor_get_status(void);
int xy_sensor_set_period(unsigned int period_ms);
int hal_uart_init(void);
int osal_mutex_lock(void);
```

**不正确：** 公共名称无法表达所属模块或操作含义。

```c
int init(void);
int sensor_do_it(void);
int SetPeriod(unsigned int value);
```

**可选正确：** 已发布接口可以保留原名，通过明确适配提供新入口。

```c
int sensor_start(void);

int xy_sensor_start(void)
{
    return sensor_start();
}
```

### 2.3 私有函数和状态限制可见性

不跨模块使用的函数与状态**必须**使用 `static`。私有函数、状态和只读对象均**应该**小写加下划线；
可用 `prv_`、`s_`、`k_` 区分用途，确有外部链接需求的全局变量可用 `g_`。
前缀不能替代 `static` 的可见性约束，也不要求为添加前缀而重命名已有接口。

**正确：**

```c
static int s_initialized;
static const unsigned int k_max_retries = 3;
extern unsigned int g_system_flags;

static void prv_reset_state(void)
{
    s_initialized = 0;
}
```

**不正确：** 内部状态暴露为可被任意模块修改的外部符号。

```c
int initialized;

void reset_state(void)
{
    initialized = 0;
}
```

**可选正确：** 状态只服务于一个函数时，进一步缩小作用域。

```c
unsigned int xy_next_sequence(void)
{
    static unsigned int s_sequence;
    return ++s_sequence;
}
```

### 2.4 类型、宏、枚举成员和回调统一命名

类型、结构体标签、宏、枚举成员和回调均**应该**小写加下划线，并使用模块前缀避免冲突。
typedef 类型推荐 `_t` 后缀；结构体标签也可以带 `_t`，允许与 typedef 同名。
枚举成员小写或全大写均可，同一组枚举保持一致；宏推荐小写，不据此批量重命名已有宏体系。
函数指针 typedef 推荐 `_fn`，回调实现推荐 `_callback`。
第三方库和官方／权威 SDK 继续保持原有命名、花括号及注释风格，自有适配层采用本文约定。
普通单行注释用 `/* ... */`，成员文档用 `/**< ... */`；需要防止注释折行或对齐变化时，
局部配对使用 `/* clang-format off */` 与 `/* clang-format on */`，具体示例见第 6.1 节。

**正确：**

```c
#define xy_sensor_capacity 16

typedef enum {
    xy_sensor_idle = 0,
    xy_sensor_ready,
} xy_sensor_state_t;

typedef struct xy_sensor_node_t {
    struct xy_sensor_node_t* next;
} xy_sensor_node_t;

typedef void (*xy_sensor_callback_fn)(int status);
void sensor_ready_callback(int status);
```

**不正确：** 混用驼峰，且同组枚举成员的命名风格不一致。

```c
#define sensorCapacity 16

typedef enum {
    xy_sensor_idle,
    XY_SENSOR_READY,
} SensorState;

typedef void (*Callback)(int status);
```

**可选正确：** 枚举成员整组使用全大写；结构体标签也可不带 `_t`，简单对象可用匿名类型。

```c
typedef enum {
    XY_SENSOR_IDLE = 0,
    XY_SENSOR_READY,
} xy_sensor_state_t;

typedef struct xy_point {
    int x;
    int y;
} xy_point_t;

typedef union {
    unsigned int count;
    float ratio;
} xy_value_t;
```

### 2.5 测试名描述被测行为

测试函数**应该**使用 `test_<模块>_<行为>`，名称说明触发条件或预期结果。

**正确：**

```c
void test_sensor_init_rejects_null_config(void);
void test_pid_reset_clears_integral(void);
```

**不正确：** 无法判断测试覆盖什么行为。

```c
void test1(void);
void test_sensor_ok(void);
```

**可选正确：** 对边界场景明确标出边界。

```c
void test_buffer_write_rejects_full_capacity(void);
```

### 2.6 跨模块内部接口标明内部用途

**必须**通过明确声明使用跨编译单元接口；**应该**用 `<模块>_int_` 区分库内部接口与稳定公共接口。
**禁止**把仍被其他编译单元调用的函数错误地改成 `static`，或仅通过前缀假定接口已隐藏。

**正确：** 内部前缀表达用途，实际可见性由声明与链接方式控制。

```c
int xy_sensor_int_calibrate(unsigned int reference);
```

**不正确：** 调用方只能看到一个没有实现的私有声明。

```c
static int xy_sensor_int_calibrate(unsigned int reference);
```

**可选正确：** 确实只在一个编译单元使用的实现，保留为私有函数。

```c
static int prv_calibrate(unsigned int reference)
{
    return reference > 0 ? 0 : -1;
}
```

## 3. 排版

### 3.1 使用 4 空格缩进

每级缩进**必须**使用 4 个空格，**禁止**使用 Tab。续行基础缩进为 4 个空格，括号后的参数允许按括号对齐。

**正确：**

```c
if (is_ready) {
    count += 1;
}
```

**不正确：** 使用 2 空格缩进；下例的实际缩进只有 2 个空格。

```c
if (is_ready) {
  count += 1;
}
```

**可选正确：** 长表达式续行并按层级缩进。

```c
if (is_initialized && has_available_capacity && is_transport_connected && is_configuration_valid &&
    is_sampling_enabled) {
    count += 1;
}
```

### 3.2 空格区分关键字与调用

控制关键字与 `(` 之间**必须**留 1 个空格，函数名与 `(` 之间**禁止**留空格。
二元运算符两侧、逗号后留空格；括号与方括号内部不额外留空格。

**正确：**

```c
if (count > 0) {
    total = sum(values[0], count);
}
```

**不正确：**

```c
if( count>0 ) {
    total=sum (values[ 0 ],count);
}
```

**可选正确：** 一元运算符紧邻操作数。

```c
if (!is_ready) {
    ++retry_count;
}
```

### 3.3 函数花括号单独成行，控制分支始终加括号

函数定义的左花括号**必须**单独一行，右花括号也单独一行，空函数同样适用。
结构体、枚举、联合体和控制语句的左花括号与声明同行。单条分支也使用花括号；
`else` 与前一个 `}` 同行，`do` 循环以 `} while (...);` 结束。

**正确：**

```c
int xy_normalize(int value)
{
    if (value < 0) {
        return 0;
    } else {
        return value;
    }
}
```

**不正确：** 函数左花括号与声明同行，且条件分支缺少花括号。

```c
int xy_normalize(int value) {
    if (value < 0)
        return 0;
    return value;
}
```

**可选正确：** 提前返回可以减少嵌套，仍保留花括号。

```c
int xy_normalize(int value)
{
    if (value < 0) {
        return 0;
    }

    return value;
}
```

### 3.4 函数、循环与分支不压成单行

**禁止**将函数压成单行，空函数也**必须**将左右花括号分别放在单独一行。
循环与分支不压成单行；空循环通过注释说明等待条件，避免裸分号掩盖意图。

**正确：**

```c
void xy_clear_count(unsigned int* count)
{
    *count = 0;
}
```

**不正确：**

```c
void xy_clear_count(unsigned int* count) { *count = 0; }
void xy_empty_callback(void) {}
while (is_busy);
```

**可选正确：** 必须保留的空回调也使用独立花括号；轮询条件必须重新读取可变化的状态，
其中 `status` 指向 `volatile` 硬件寄存器。

```c
void xy_empty_callback(void)
{
}

while ((*status & 1U) != 0) {
    /* Poll a volatile hardware status register. */
}
```

### 3.5 指针靠类型，强制转换后不加空格

指针星号**必须**靠类型；C 风格转换后紧接表达式。只读数据的指针**必须**保留 `const`。

**正确：**

```c
const uint8_t* data;
uint8_t value = (uint8_t)raw_value;
```

**不正确：** 排版不符合约定，并去掉只读限定。

```c
uint8_t *data;
uint8_t value = (uint8_t) raw_value;
```

**可选正确：** 指针本身不应改变时，另外限定指针对象。

```c
const uint8_t* const data = input;
```

### 3.6 控制行宽，允许参数在行宽内合并

行宽**应该**以 100 列为目标。参数可在行宽内合并；过长表达式应换行或提取有意义的中间结果，
不通过缩写成难懂名称规避行宽。不可安全拆分的字面量按语义处理。

**正确：**

```c
int xy_clamp(int value, int minimum, int maximum);
```

**不正确：** 将多个逻辑和分支挤在同一行，即使编译器可以接受也难以阅读。

```c
int xy_clamp(int v, int a, int b) { if (v < a) { return a; } if (v > b) { return b; } return v; }
```

**可选正确：** 复杂判断可以提取为有含义的布尔值。

```c
bool is_out_of_range = value < minimum || value > maximum;
if (is_out_of_range) {
    return -1;
}
```

### 3.7 switch 分支缩进并明确退出方式

`case` **必须**相对 `switch` 缩进一级，语句再缩进一级。保留 `default`；分支局部变量使用块作用域，
有意贯穿必须在后续 `case` 前加 `/* go through */`；兼容按注释识别贯穿的质量工具时，
紧接着再写 `/* fall through */`。这些注释说明有意贯穿，不用于掩盖遗漏的 `break`。

**正确：**

```c
switch (mode) {
    case 0:
        count += 1;
        /* go through */
        /* fall through */
    case 1:
        count += 2;
        break;
    default:
        break;
}
```

**不正确：** 缩进缺失，且无意贯穿造成两次修改。

```c
switch (mode) {
case 0:
    count += 1;
case 1:
    count += 2;
}
```

**可选正确：** 行为相同的空标签可以共享实现，仍标明贯穿；分支需要局部变量时使用块。

```c
switch (mode) {
    case 0:
        /* go through */
        /* fall through */
    case 1: {
        int result = 1;
        return result;
    }
    default:
        return -1;
}
```

### 3.8 空行分隔逻辑，预处理指令不缩进

连续空行**必须**最多保留 1 行；用空行区分检查与处理阶段。预处理指令从行首开始，不随嵌套增加缩进。

**正确：**

```c
#if defined(xy_feature)
#if defined(xy_trace)
#define xy_trace_enabled 1
#endif
#endif
```

**不正确：** 指令层级用额外空格模拟代码块缩进。

```c
#if defined(xy_feature)
    #if defined(xy_trace)
        #define xy_trace_enabled 1
    #endif
#endif
```

**可选正确：** 较长条件块使用注释说明结束位置。

```c
#if defined(xy_trace)
static unsigned int s_trace_count;
#endif /* xy_trace */
```

### 3.9 链式条件使用 else if，三元表达式用于取值

**必须**用明确的条件分支表达互斥操作；**应该**用 `else if` 表达互斥条件链。
**禁止**只用三元表达式选择有副作用的操作；**可以**用简单三元表达式产生返回值、实参或赋值结果。

**正确：**

```c
if (value < 0) {
    state = -1;
} else if (value > 0) {
    state = 1;
} else {
    state = 0;
}
```

**不正确：** 结果被丢弃，表达式仅用于隐藏两个赋值分支。

```c
is_ready ? (state = 1) : (state = 0);
```

**可选正确：** 简单取值可以使用三元表达式。

```c
int state = is_ready ? 1 : 0;
```

### 3.10 按迭代含义选择循环

**应该**对有界计数使用 `for`，对条件驱动使用 `while`，确需先执行一次时使用 `do-while`。
**应该**在循环内声明计数器，独立递增时优先使用 `++i`；**禁止**使用恒真的无符号终止条件。
循环选择取决于语义，不强制将所有循环替换为同一种形式。

**正确：**

```c
for (size_t i = 0; i < count; ++i) {
    values[i] = 0;
}
```

**不正确：** 无符号变量始终大于等于 0，递减到 0 后将回绕。

```c
for (size_t i = count; i >= 0; --i) {
    values[i] = 0;
}
```

**可选正确：** 条件驱动时使用 `while`；先检查容量后，后自增可以表达保留旧索引的意图。

```c
while (count < capacity) {
    values[count++] = 0;
}
```

### 3.11 表达式不依赖求值顺序，副作用单独表达

**必须**保证表达式在 C99 允许的求值顺序下具有一致含义；**禁止**在同一个未排序的表达式中
多次修改同一对象，或同时修改并读取它。**应该**将状态修改与条件判断分开，并用括号明确混合运算的含义。

**正确：** 先完成一次状态修改，再使用更新后的值。

```c
index += 1;
int total = index + offset;

if ((flags & mask) != 0) {
    is_ready = true;
}
```

**不正确：** 第一行含未排序的修改与读取；第二行按错误优先级组合比较和按位操作。

```c
int total = index++ + index;
if (flags & mask == 0) {
    is_ready = false;
}
```

**可选正确：** 明确需要旧值时，先保存旧值，再独立修改对象。

```c
size_t previous_index = index;
index += 1;
int total = values[previous_index] + offset;
```

## 4. 声明与类型

### 4.1 每行声明一个变量，在最小作用域初始化

**应该**每行声明一个变量，**禁止**在同一声明中混淆指针和非指针。变量在使用前初始化；C99 允许声明与语句混合。

**正确：**

```c
int count = 0;
int* current = &count;
int* previous = NULL;
```

**不正确：** `previous` 实际不是指针；`count` 未初始化便读取。

```c
int count;
int* current, previous;
int total = count + 1;
```

**可选正确：** 只在分支中使用的变量放入分支。

```c
if (is_ready) {
    int retry_count = 0;
    retry_count += 1;
}
```

### 4.2 明确只读、空参数与空指针语义

只读输入**必须**使用 `const`；无参数函数写 `(void)`；指针有效性使用与 `NULL` 的显式比较。

**正确：**

```c
int xy_get_status(void);

int xy_read_first(const uint8_t* data)
{
    if (data == NULL) {
        return -1;
    }

    return data[0];
}
```

**不正确：** 空括号未声明参数约束，并且先解引用后检查。

```c
int xy_get_status();

int xy_read_first(uint8_t* data)
{
    int value = data[0];
    if (!data) {
        return -1;
    }

    return value;
}
```

**可选正确：** 检查多个只读输入。

```c
if (left == NULL || right == NULL) {
    return -1;
}
```

### 4.3 布尔值直接判断

新代码**应该**使用 C99 布尔类型；**禁止**将布尔语义错误地写成与 `1` 比较。计数和长度使用明确的数值比较。

**正确：**

```c
bool is_ready = true;
if (is_ready && length > 0) {
    count += 1;
}
```

**不正确：** 非零整数未必恰好等于 1。

```c
int is_ready = 2;
if (is_ready == 1) {
    count += 1;
}
```

**可选正确：** 必须兼容整数布尔接口时先归一化。

```c
bool is_ready = raw_status != 0;
if (!is_ready) {
    return -1;
}
```

### 4.4 位宽、长度和静态初始化表达真实含义

协议字段**必须**使用固定宽度整数，对象长度使用 `size_t`。静态存储对象自动清零；
非零初始值可以直接给出，运行期配置另行初始化。

**正确：**

```c
static uint32_t s_packet_count;
static unsigned int s_retry_limit = 3;
size_t capacity = sizeof(buffer);
uint16_t wire_length = 128;
```

**不正确：** 假定普通整数宽度固定，或用过窄类型保存对象大小。

```c
unsigned long wire_value; /* Incorrectly assumes exactly 32 bits. */
uint8_t buffer[300];
uint8_t capacity = sizeof(buffer);
```

**可选正确：** 只读配置表使用静态常量。

```c
static const uint16_t k_sample_periods_ms[] = {10, 20, 50};
```

### 4.5 结构体成员逐行声明，复杂对象指定初始化

结构体成员**必须**逐行声明；复杂对象**应该**使用指定初始化器和尾随逗号。

**正确：**

```c
typedef struct {
    unsigned int baud_rate;
    unsigned int stop_bits;
} xy_uart_config_t;

xy_uart_config_t config = {
    .baud_rate = 115200,
    .stop_bits = 1,
};
```

**不正确：** 成员挤在一行，位置初始化难以看出数值含义。

```c
typedef struct { unsigned int baud_rate, stop_bits; } xy_uart_config_t;
xy_uart_config_t config = {115200, 1};
```

**可选正确：** 仅需要将普通整数成员全部清零时，可以使用零初始化。

```c
xy_uart_config_t config = {0};
```

### 4.6 通用数据指针保留 const，C 分配结果不强制转换

**必须**用 `void*` 表达接收任意对象类型的可写数据接口，只读数据使用 `const void*`；
已知类型的接口**应该**保留具体指针类型。**禁止**在 C 代码中对 `malloc` 返回值作多余强制转换，
或通过转换去掉只读输入的 `const`；C++ 互操作不照搬 C 的分配写法。

**正确：**

```c
int xy_inspect_bytes(const void* data, size_t length);
int xy_fill_bytes(void* data, size_t length);

int* value = malloc(sizeof(*value));
if (value == NULL) {
    return -1;
}
free(value);
```

**不正确：** 返回值转换没有必要，并且把只读对象当作可写对象修改。

```c
int* value = (int*)malloc(sizeof(int));
const int limit = 10;
int* writable = (int*)&limit;
*writable = 20;
```

**可选正确：** 明确处理整数对象时采用具体类型，固定只读值使用 `const`。

```c
const unsigned int retry_limit = 3;
int xy_inspect_value(const int* value);
```

## 5. 公共声明、宏与模块边界

### 5.1 公共声明提供包含保护与 C++ 链接兼容

可重复引入的公共声明**必须**有包含保护；C 函数声明在 C++ 环境下必须使用 C 链接。
下面直接展示声明，不引用任何头文件。

**正确：**

```c
#ifndef xy_sensor_declarations
#define xy_sensor_declarations

#ifdef __cplusplus
extern "C" {
#endif

int xy_sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* xy_sensor_declarations */
```

**不正确：** 未保护重复声明的类型，且纯 C 编译器无法识别无条件的 `extern "C"`。

```c
typedef struct {
    int value;
} xy_sample_t;

extern "C" int xy_sensor_init(void);
```

**可选正确：** 保留相同的链接保护，包含保护改用 `defined` 判断。

```c
#if !defined(xy_sensor_declarations)
#define xy_sensor_declarations

#ifdef __cplusplus
extern "C" {
#endif

int xy_sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* xy_sensor_declarations */
```

### 5.2 表达式宏加括号，避免重复求值

表达式宏**必须**给参数和整体加括号；**禁止**向可能重复求值的宏传入有副作用的实参。
优先用有类型的 `static inline` 实现复杂运算。

**正确：**

```c
#define xy_square(value) ((value) * (value))

int value = 3;
int result = xy_square(value);
```

**不正确：** 缺少括号，或让同一变量被多次修改，可能触发未定义行为。

```c
#define xy_square(value) value * value
int result = xy_square(1 + 2);
int invalid = xy_square(index++);
```

**可选正确：** 对已知不会溢出的输入，内联函数只计算一次实参。

```c
static inline unsigned int xy_square(unsigned int value)
{
    return value * value;
}

unsigned int result = xy_square(3);
```

### 5.3 多语句宏使用 do-while 包装

多语句宏**必须**用 `do { ... } while (0)` 包装。每个需要续接的物理行末必须写反斜杠 `\`，
最后一行不写 `\`，也不在宏定义尾部附加调用者的分号；调用时由调用者写分号。
反斜杠必须是该行最后一个字符，后面不能跟空格或注释。LF 与 CRLF 均可作为物理换行。
下例缩短反斜杠的对齐距离，避免续行符在较窄的阅读界面中落到视野之外。

**正确：**

```c
#define xy_clear_pair(first, second) \
    do {                             \
        (first) = 0;                 \
        (second) = 0;                \
    } while (0)
```

**不正确：** 第一条赋值末尾缺少 `\`，宏在该物理行结束，剩余代码不再属于宏。

```c
#define xy_clear_pair(first, second) \
    do {                            \
        (first) = 0;
        (second) = 0;               \
    } while (0)
```

**可选正确：** 类型固定时使用内联函数，并由接口约定非空指针。

```c
static inline void xy_clear_pair(int* first, int* second)
{
    *first = 0;
    *second = 0;
}
```

### 5.4 区分宏已定义与值为真

存在型开关**应该**用 `defined`，取值型开关**必须**检查值。**禁止**把值为 0 的已定义宏当成启用状态。

**正确：**

```c
#if defined(xy_feature_enabled) && xy_feature_enabled
int xy_feature_start(void);
#endif
```

**不正确：** 开关定义为 0 时，声明依然进入启用分支。

```c
#define xy_feature_enabled 0
#ifdef xy_feature_enabled
int xy_feature_start(void);
#endif
```

**可选正确：** 约定「只定义即启用」的存在型开关可以只检查定义。

```c
#if defined(xy_has_diagnostics)
int xy_diagnostics_start(void);
#endif
```

### 5.5 通过抽象接口隔离平台

应用**必须**依赖组件，组件通过 HAL、OSAL 等抽象访问硬件和操作系统。
不把供应商寄存器操作混入通用业务逻辑。

**正确：** 通过传入的读取接口获取数据。

```c
typedef int (*xy_read_fn)(void* context);

int xy_read_sample(xy_read_fn read_value, void* context)
{
    if (read_value == NULL) {
        return -1;
    }

    return read_value(context);
}
```

**不正确：** 通用组件硬编码硬件地址和寄存器布局。

```c
int xy_read_sample(void)
{
    return *(volatile unsigned int*)0x40000000;
}
```

**可选正确：** 固定平台适配层可以封装寄存器访问，业务层仍只调用适配接口。

```c
int hal_sample_read(void);

int xy_read_sample(void)
{
    return hal_sample_read();
}
```

### 5.6 公共声明自包含，定义与实现保持唯一

**必须**为公共函数提供原型，在声明使用类型前提供必要的类型声明；公共变量用 `extern` 声明。
**禁止**在可重复引入的公共声明中定义可变全局对象、暴露模块私有实现，或直接包含另一个实现单元。
**应该**先验证自身接口的声明完整性，再按基础类型、项目接口、外部接口分组引入必要依赖；
组内排序交给工具。示例直接展示声明内容，不引用具体文件。

**正确：**

```c
typedef struct xy_config_t {
    unsigned int period_ms;
} xy_config_t;

extern xy_config_t g_config;
int xy_config_apply(const xy_config_t* config);
```

**不正确：** 依赖调用方事先定义类型，并将全局定义与私有函数混入公共声明。

```c
xy_config_t g_config;
static void prv_reset_config(void);
```

**可选正确：** 调用方只持有句柄时，可以只公开不完整类型与操作接口。

```c
typedef struct xy_context_t xy_context_t;

int xy_context_reset(xy_context_t* context);
```

### 5.7 可配置常量集中定义，保留外部覆盖能力

**应该**集中管理功能开关和默认常量；**必须**在依赖不满足时报告配置错误。
**禁止**在业务流程中重新定义开关以绕过依赖，或无条件覆盖集成方提供的配置。

**正确：**

```c
#if !defined(xy_retry_limit)
#define xy_retry_limit 3
#endif

#if defined(xy_feature_enabled) && xy_feature_enabled
#if !defined(xy_transport_enabled) || !xy_transport_enabled
#error "The feature requires transport support."
#endif
#endif
```

**不正确：** 无条件覆盖上层配置，隐藏依赖缺失。

```c
#undef xy_retry_limit
#define xy_retry_limit 100
#define xy_transport_enabled 1
```

**可选正确：** 运行期可调选项可以使用显式配置对象。

```c
typedef struct xy_retry_config_t {
    unsigned int retry_limit;
    unsigned int timeout_ms;
} xy_retry_config_t;
```

## 6. 注释、接口与错误处理

### 6.1 注释解释原因，格式统一

标识符使用英文；注释可以使用中文或英文，同一模块保持一致。
普通说明**必须**使用 `/* ... */`，**禁止**在自有代码中用 `//` 代替普通说明；多行注释每行以 ` *` 开头；紧跟结构体／枚举成员的 Doxygen
成员文档使用 `/**< ... */`，两者不可混淆。行尾注释与代码至少间隔 1 个空格。
当普通单行注释或成员注释需要保留同行关系、对齐或完整一行时，用配对的
`/* clang-format off */`、`/* clang-format on */` 保护最小范围，避免格式化导致换行。
受保护的注释必要时可以超过 100 列；保护结束后立即恢复格式化，不关闭整个模块的格式化。

**正确：**

```c
/* clang-format off */
size_t usable_capacity = capacity - 1; /* Leave one slot unused to distinguish full from empty. */
/* clang-format on */

/* clang-format off */
typedef struct xy_wait_config_t {
    unsigned int timeout_ms; /**< Maximum blocking time in milliseconds; zero requests a nonblocking attempt. */
} xy_wait_config_t;
/* clang-format on */
```

**不正确：** 普通说明误用成员标记，成员文档又写成普通注释；格式保护没有配对恢复。

```c
/* clang-format off */
/**< This is a standalone explanation, not member documentation. */
typedef struct {
    unsigned int timeout_ms; /* Intended as API member documentation, but uses an ordinary comment. */
} xy_wait_config_t;
```

**可选正确：** 不需要保留同行的普通说明可以放在独立多行注释中，无需关闭格式化。

```c
/*
 * The device needs one idle cycle after reset.
 * The first sample is intentionally discarded.
 */
```

### 6.2 公共接口文档说明契约

**必须**为公共类型以及每个函数提供 Doxygen 说明，包括 `static` 函数。
公共接口契约在声明处说明，私有函数契约在实现处说明；**应该**避免复制两份容易失步的完整说明。
函数说明包含用途、参数方向和非 void 返回值；
缓冲区接口还说明长度单位、所有权、生命周期、空指针和阻塞行为。
**必须**保留适用的普通块注释许可证声明，单元说明使用不带路径的 `@file` 与 `@brief`，
说明后留一空行；**应该**在参数名与描述间使用冒号，补充必要的 `@note`、`@warning`。
**禁止**编造版本号或作者信息来填充文档。

**正确：**

```c
/**
 * @file
 * @brief Synchronous sample processing interface.
 */

/**
 * @brief Process bytes without retaining the input buffer.
 * @param[in] data: Borrowed input; must not be NULL.
 * @param[in] length: Number of bytes; must be greater than zero.
 * @return 0 on success, or a negative error code.
 * @note Does not block and does not retain data after returning.
 */
int xy_process(const uint8_t* data, size_t length);
```

**不正确：** 未解释长度单位、返回值和是否持有数据。

```c
/* Process data. */
int xy_process(const uint8_t* data, size_t length);
```

**可选正确：** 输出参数明确容量和写入语义。

```c
/**
 * @brief Read one sample without blocking.
 * @param[out] value: Non-NULL destination, written only on success.
 * @return 0 on success, or a negative error code.
 */
int xy_read_sample(int* value);
```

私有函数同样记录输入范围和失败语义，示例没有因为 `static` 而省略函数文档。

```c
/**
 * @brief Double a validated sample value.
 * @param[in] value: Input value in the range 0 through 100.
 * @return Twice the input value, or -1 when the input is outside the range.
 */
static int prv_scale_value(int value)
{
    if (value < 0 || value > 100) {
        return -1;
    }

    return value * 2;
}
```

### 6.3 函数保持单一职责

**应该**将函数控制在 50 行以内，参数不超过 5 个，嵌套不超过 4 层，圈复杂度不超过 10；
这些是审查目标，不是 C 语法限制。超过 80 行时触发长度提醒，超出其他目标时也需审查原因。
**必须**按职责拆分复杂逻辑；**禁止**仅为压低行数创建无意义的转发函数。
相关参数可以合并为有明确语义的配置对象，重复的大段逻辑应该提取公共实现。

**正确：** 验证和转换可以分别表达。

```c
static int prv_is_valid(int value)
{
    return value >= 0 && value <= 100;
}

int xy_scale(int value)
{
    if (!prv_is_valid(value)) {
        return -1;
    }

    return value * 2;
}
```

**不正确：** 一个操作混入无关的硬件维护和系统管理。

```text
process_sample:
    输入 7 个含义不相关的参数，嵌套超过 4 层
    校验采样值
    写入采样队列
    擦除配置存储
    重启通信模块
    更新显示布局
```

**可选正确：** 简单逻辑直接实现，不强行拆分。

```c
int xy_scale(int value)
{
    if (value < 0 || value > 100) {
        return -1;
    }

    return value * 2;
}
```

### 6.4 返回值区分状态与数据

状态接口**必须**以 0 表示成功、负值表示错误，使用具名常量。
返回计数、布尔值或枚举的接口遵守明确契约，不将它们与状态码混用。

**正确：**

```c
enum {
    xy_ok = 0,
    xy_error_invalid_param = -1,
};

int xy_validate(int value)
{
    if (value < 0) {
        return xy_error_invalid_param;
    }

    return xy_ok;
}
```

**不正确：** 数据值 0 与失败使用相同结果，调用方无法区分。

```c
int xy_get_temperature(bool is_valid, int temperature)
{
    return is_valid ? temperature : 0;
}
```

**可选正确：** 使用状态返回值加输出参数。

```c
int xy_get_temperature(bool is_valid, int temperature, int* output)
{
    if (!is_valid || output == NULL) {
        return -1;
    }

    *output = temperature;
    return 0;
}
```

### 6.5 公共入口先检查再访问

公共入口**必须**检查指针、长度、范围和状态；可恢复的输入错误通过返回值处理，**禁止**只依赖断言。

**正确：**

```c
int xy_first_byte(const uint8_t* data, size_t length)
{
    if (data == NULL || length == 0) {
        return -1;
    }

    return data[0];
}
```

**不正确：** 未检查长度，且禁用断言后不再保护指针。

```c
int xy_first_byte(const uint8_t* data, size_t length)
{
    assert(data != NULL);
    return data[0];
}
```

**可选正确：** 允许空输入的接口可以显式定义空长度为成功。

```c
int xy_validate_input(const uint8_t* data, size_t length)
{
    if (length == 0) {
        return 0;
    }
    if (data == NULL) {
        return -1;
    }

    return 0;
}
```

### 6.6 失败路径完整释放资源

**必须**处理可能失败的调用，按获取顺序的逆序释放资源，保留有意义的错误结果。
下面的分配和释放遵守标准 C 的语义。

**正确：**

```c
int* first = malloc(sizeof(*first));
if (first == NULL) {
    return -1;
}

int* second = malloc(sizeof(*second));
if (second == NULL) {
    free(first);
    return -1;
}

free(second);
free(first);
```

**不正确：** 第二次分配失败时泄漏第一次分配的对象。

```c
int* first = malloc(sizeof(*first));
int* second = malloc(sizeof(*second));
if (second == NULL) {
    return -1;
}
```

**可选正确：** 多阶段获取可以使用集中清理；初始化指针后允许安全释放空指针。

```c
int xy_allocate_pair(void)
{
    int result = -1;
    int* first = NULL;
    int* second = NULL;

    first = malloc(sizeof(*first));
    if (first == NULL) {
        goto cleanup;
    }
    second = malloc(sizeof(*second));
    if (second == NULL) {
        goto cleanup;
    }
    result = 0;

cleanup:
    free(second);
    free(first);
    return result;
}
```

### 6.7 日志通过统一接口，携带诊断上下文

**必须**通过统一的 `xy_log_e/w/i/d/v` 日志接口或保持相同日志契约的适配回调记录操作与状态；
**应该**按错误、警告、信息、调试与详细跟踪选择级别，只在需要的位置记录足够上下文。
**禁止**业务逻辑直接用 `printf`、`puts` 输出日志，避免上下层重复打印同一错误。

**正确：**

```c
void xy_log_e(const char* format, ...);

if (result < 0) {
    xy_log_e("sensor_start failed: status=%d", result);
    return result;
}
```

**不正确：** 缺少状态和来源，且直接使用控制台输出。

```c
if (result < 0) {
    printf("failed");
}
```

**可选正确：** 将日志回调注入组件，便于替换后端和测试。

```c
typedef void (*xy_log_fn)(const char* operation, int status);

if (result < 0 && log_error != NULL) {
    log_error("sensor_start", result);
}
```

### 6.8 宏文档说明含义、单位与初始化显示

**必须**为公开宏提供 Doxygen 说明，写明取值含义或单位，使用 `@hideinitializer`
控制文档中的宏展开显示；**应该**对纯数值说明实际业务含义。
**禁止**把没有上下文的数值当作可理解的公开接口。

**正确：**

```c
/**
 * @brief Maximum number of retransmissions after the first attempt.
 * @hideinitializer
 */
#define xy_retry_limit 3
```

**不正确：** 宏名与注释都无法说明数值代表什么。

```c
/* A value. */
#define xy_value 3
```

**可选正确：** 不需要预处理运算的私有常量可以使用有类型的只读对象。

```c
static const unsigned int k_retry_limit = 3;
```

## 7. 内存、安全与可移植性

### 7.1 不使用 VLA，控制栈开销

**禁止**可变长度数组和无界递归，避免大栈对象。
优先使用有界缓冲区或调用方提供的存储；动态分配必须符合目标平台的资源策略。
**应该**结合线程栈、调用深度和中断嵌套预算确定局部缓冲区上限，不能把「小于 1 KB」
当作普遍安全保证，也不能把换成堆分配当作唯一解决方法。

**正确：**

```c
enum {
    xy_buffer_capacity = 64,
};

uint8_t buffer[xy_buffer_capacity];
if (length > sizeof(buffer)) {
    return -1;
}
```

**不正确：** 由外部输入直接决定栈空间。

```c
uint8_t buffer[length];
```

**可选正确：** 调用方提供存储，接口检查容量。

```c
int xy_check_capacity(uint8_t* buffer, size_t capacity, size_t length)
{
    if (buffer == NULL || length > capacity) {
        return -1;
    }

    return 0;
}
```

### 7.2 不返回失效对象，不在释放后访问

对象的有效期**必须**覆盖使用期。**禁止**释放后访问和重复释放；
将某一指针设为 `NULL` 不能修复其他别名中的悬空指针。

**正确：**

```c
int* value = malloc(sizeof(*value));
if (value == NULL) {
    return -1;
}

*value = 42;
free(value);
value = NULL;
```

**不正确：** 返回局部变量的地址。

```c
int* xy_get_value(void)
{
    int value = 42;
    return &value;
}
```

**可选正确：** 将结果写入调用方提供的对象。

```c
int xy_get_value(int* output)
{
    if (output == NULL) {
        return -1;
    }

    *output = 42;
    return 0;
}
```

### 7.3 边界与溢出在运算前检查

索引**必须**小于容量。加法和乘法在执行前验证范围，**禁止**依赖有符号溢出，
无符号回绕仅用于明确设计并记录的算法。

**正确：**

```c
if (index >= capacity) {
    return -1;
}
if (count > UINT32_MAX - increment) {
    return -1;
}

buffer[index] = 0;
count += increment;
```

**不正确：** 允许 `index == capacity`，并在可能溢出后才比较结果。

```c
if (index <= capacity) {
    buffer[index] = 0;
}
if (count + increment > UINT32_MAX) {
    return -1;
}
```

**可选正确：** 数组分配前检查乘法；此处 `count` 与 `element_size` 均为 `size_t`。

```c
if (element_size == 0 || count > SIZE_MAX / element_size) {
    return -1;
}

size_t bytes = count * element_size;
```

### 7.4 字符串预留终止符并处理截断

**必须**检查目标容量，确保终止符位于有效范围内；带长度的字符串函数同样需要检查结果。
以下复制示例要求源对象覆盖所读取的长度，且源与目标不重叠；重叠复制应使用 `memmove`。

**正确：** `length` 表示有效字符数，不包含终止符。

```c
if (destination == NULL || source == NULL || length >= capacity) {
    return -1;
}

memcpy(destination, source, length);
destination[length] = '\0';
```

**不正确：** 复制满容量后，在容量之外写终止符。

```c
memcpy(destination, source, capacity);
destination[capacity] = '\0';
```

**可选正确：** 非重叠、已终止的源字符串可以使用格式化函数，并检查截断。

```c
if (destination == NULL || source == NULL || capacity == 0) {
    return -1;
}

int written = snprintf(destination, capacity, "%s", source);
if (written < 0 || (size_t)written >= capacity) {
    return -1;
}
```

宽字符字符串同样需要终止符；下面的 `capacity` 和 `length` 均以宽字符元素为单位，
源对象覆盖读取范围，源与目标不重叠。

```c
if (destination == NULL || source == NULL || length >= capacity) {
    return -1;
}

wmemcpy(destination, source, length);
destination[length] = L'\0';
```

### 7.5 缩窄与移位检查取值范围

缩窄前**必须**验证目标类型可表达的范围；移位数必须小于提升后左操作数的位宽，
位运算使用宽度适当的无符号类型。

**正确：**

```c
if (raw_value > UINT8_MAX || bit_index >= 32) {
    return -1;
}

uint8_t value = (uint8_t)raw_value;
uint32_t mask = UINT32_C(1) << bit_index;
```

**不正确：** 未检查截断，并且对 32 位对象移位 32 位无效。

```c
uint8_t value = (uint8_t)raw_value;
uint32_t mask = UINT32_C(1) << 32;
```

**可选正确：** 输入为有符号数时，同时验证下界。

```c
if (raw_value < 0 || raw_value > UINT8_MAX) {
    return -1;
}

uint8_t value = (uint8_t)raw_value;
```

### 7.6 协议显式编码，不依赖内存布局

**禁止**把结构体填充、枚举宽度、联合体字节视图和位域顺序作为跨平台协议约定。
协议和持久化数据使用明确的字段宽度与字节顺序。

**正确：** `value` 为 `uint16_t`，明确编码为低字节在前；输出容量至少为 2 字节。

```c
output[0] = (uint8_t)(value & UINT16_C(0xFF));
output[1] = (uint8_t)(value >> 8);
```

**不正确：** 默认联合体的第一个字节在所有平台上都是最低字节。

```c
union {
    uint16_t value;
    uint8_t bytes[2];
} view;

view.value = value;
output[0] = view.bytes[0];
```

**可选正确：** 协议要求高字节在前时显式反转顺序。

```c
output[0] = (uint8_t)(value >> 8);
output[1] = (uint8_t)(value & UINT16_C(0xFF));
```

### 7.7 并发访问使用真实同步

**禁止**把 `volatile` 当作原子性或线程同步保证。共享计数和可能共用存储单元的位域**必须**同步，
锁或临界区需覆盖完整的读改写操作。

**正确：** 同一把互斥锁保护所有访问，以下片段位于允许阻塞的线程上下文。

```c
void osal_lock(void);
void osal_unlock(void);

osal_lock();
shared_count += 1;
osal_unlock();
```

**不正确：** 声明为 `volatile` 后仍然可能丢失其他线程的更新。

```c
static volatile unsigned int s_shared_count;

void xy_increment_count(void)
{
    s_shared_count += 1;
}
```

**可选正确：** 单核平台与中断共享时，使用能保存并恢复原中断状态的短临界区；
该方式不用于多核同步。

```c
unsigned int osal_irq_save(void);
void osal_irq_restore(unsigned int previous_state);

unsigned int previous_state = osal_irq_save();
shared_count += 1;
osal_irq_restore(previous_state);
```

### 7.8 多锁获取顺序固定，失败时只释放已获取的锁

**必须**为可能同时使用的锁规定统一获取顺序，并按逆序释放；失败时只释放已成功获取的资源。
**禁止**重复获取同一个非递归锁，或让不同执行路径以相反顺序持有同一组锁。
**应该**缩短持锁时间，不在锁内执行无关的长时间操作。

**正确：** 获取接口用 0 表示成功，且每次成功获取都对应一次释放。

```c
if (osal_lock_first() != 0) {
    return -1;
}
if (osal_lock_second() != 0) {
    osal_unlock_first();
    return -1;
}

osal_unlock_second();
osal_unlock_first();
```

**不正确：** 已经持有第一个非递归锁时再次获取它，可能永久阻塞。

```c
osal_lock_first();
osal_lock_second();
osal_lock_first();
```

**可选正确：** 业务不需要同时持有两把锁时，可以分离临界区并保持各自数据的一致性。

```c
if (osal_lock_first() != 0) {
    return -1;
}
osal_unlock_first();

if (osal_lock_second() != 0) {
    return -1;
}
osal_unlock_second();
```

### 7.9 中断和信号上下文只执行允许的操作

**必须**区分线程、中断和异步信号上下文；**禁止**在中断中使用阻塞互斥锁，
或在异步信号处理函数中调用非异步信号安全的日志、分配和格式化操作。
**应该**只记录事件或设置标记，将复杂处理推迟到允许执行的上下文。
下面的 `sig_atomic_t` 是标准信号标记类型，不表示通用的线程同步原语。

**正确：** 标记在信号处理函数之外读取，信号处理只赋值。

```c
static volatile sig_atomic_t s_stop_requested;

void xy_signal_handler(int signal_number)
{
    (void)signal_number;
    s_stop_requested = 1;
}
```

**不正确：** 在信号处理函数中分配内存并输出日志。

```c
void xy_signal_handler(int signal_number)
{
    char* buffer = malloc(64);
    printf("signal=%d", signal_number);
    free(buffer);
}
```

**可选正确：** 中断接口可以投递事件，但接口必须明确保证中断安全且不阻塞。

```c
void osal_event_set_from_isr(unsigned int event);

void xy_device_irq_handler(void)
{
    osal_event_set_from_isr(1U);
}
```

### 7.10 sizeof 针对真实对象，数组接口显式传长度

**必须**区分数组对象与指针，分配对象时**应该**写 `sizeof(*pointer)`，减少类型修改后遗漏同步的风险。
**禁止**对已退化为指针的数组形参用 `sizeof` 推断数组容量，也禁止在 `sizeof` 中放入有副作用的表达式。
多元素分配仍必须先检查乘法溢出。

**正确：** 数组尚未退化，元素数计算有效。

```c
unsigned int values[8] = {0};
size_t count = sizeof(values) / sizeof(values[0]);

unsigned int* value = malloc(sizeof(*value));
if (value == NULL) {
    return -1;
}
free(value);
```

**不正确：** `values` 是指针，结果不是调用方的数组长度；`sizeof` 也不负责执行递增。

```c
size_t xy_count_values(const unsigned int values[])
{
    return sizeof(values) / sizeof(values[0]);
}

size_t bytes = sizeof(index++);
```

**可选正确：** 数组经由接口传递时，由调用方提供有效元素数。

```c
int xy_sum_values(const unsigned int* values, size_t count, unsigned int* total);
```

### 7.11 realloc 使用临时指针，零容量显式处理

**必须**保留 `realloc` 失败前的原指针，成功后才更新所有者持有的地址。
**禁止**把结果直接覆盖到唯一的原指针，或依赖零大小重新分配的实现差异。
成功后原指针及其派生别名均不得继续使用；新扩展的存储必须在读取前初始化。

**正确：** `buffer` 是当前唯一持有的分配地址；`new_capacity` 以字节为单位且大于零。

```c
if (new_capacity == 0) {
    return -1;
}

uint8_t* resized = realloc(buffer, new_capacity);
if (resized == NULL) {
    return -1;
}
buffer = resized;
```

**不正确：** 分配失败时丢失原分配的唯一引用，造成泄漏。

```c
buffer = realloc(buffer, new_capacity);
if (buffer == NULL) {
    return -1;
}
```

**可选正确：** 接口允许将容量降为零时，显式释放并重置所有者状态。

```c
if (new_capacity == 0) {
    free(buffer);
    buffer = NULL;
    capacity = 0;
    return 0;
}
```

### 7.12 有符号与无符号比较先统一有效范围

**必须**在把有符号值转换为大小或索引前处理负值，并确保目标类型可表示该值。
**应该**为字节数据选用明确的无符号类型；**禁止**假定普通 `char` 的符号性固定，
或依赖隐式转换让负数参与无符号长度比较。

**正确：** 先验证负值，再在两种类型都能完整表示的范围内比较；此处 `index` 为 `int`，`count` 为 `size_t`。

```c
if (index < 0 || (uintmax_t)index >= (uintmax_t)count) {
    return -1;
}

size_t position = (size_t)index;
```

**不正确：** 负值可能先转为无符号大数；普通 `char` 也未必能表示 255。

```c
if (index < count) {
    position = index;
}

char byte = 255;
```

**可选正确：** 若协议和接口天然不接受负索引，直接使用大小类型，检查上界即可。

```c
int xy_validate_position(size_t position, size_t count)
{
    if (position >= count) {
        return -1;
    }

    return 0;
}
```

## 8. 工具设置与检查

### 8.1 格式设置与正文保持一致

自动格式化**必须**以 LLVM 为基础，采用 4 空格、100 列、指针靠类型。
函数花括号单独成行，空函数也不合并；结构体和控制语句花括号同行。
使用自定义花括号策略只控制函数，不能改成让所有花括号换行的 Allman 策略。
禁止主动重排注释；需要保留成员或普通单行注释布局时，仍使用局部的 off/on 保护。
续行反斜杠靠左对齐，避免被填充到行宽边缘；换行风格优先保留输入，不强制 LF 或 CRLF。
以下直接展示设置内容，不要求读取任何配置文件。

**正确：**

```yaml
Language: Cpp
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ContinuationIndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
ReferenceAlignment: Left
BreakBeforeBraces: Custom
BraceWrapping:
  AfterFunction: true
  AfterStruct: false
  AfterEnum: false
  AfterUnion: false
  AfterControlStatement: Never
  AfterExternBlock: false
  BeforeElse: false
  BeforeWhile: false
  SplitEmptyFunction: true
AllowShortFunctionsOnASingleLine: None
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AllowShortBlocksOnASingleLine: Never
AllowShortCaseLabelsOnASingleLine: false
AllowShortEnumsOnASingleLine: false
BinPackArguments: true
BinPackParameters: true
AlignAfterOpenBracket: Align
AlignOperands: Align
IndentCaseLabels: true
IndentPPDirectives: None
MaxEmptyLinesToKeep: 1
SortIncludes: CaseInsensitive
SortUsingDeclarations: LexicographicNumeric
SpaceAfterCStyleCast: false
SpaceBeforeAssignmentOperators: true
SpaceBeforeCpp11BracedList: false
SpaceBeforeParens: ControlStatements
SpacesBeforeTrailingComments: 1
SpacesInAngles: Never
SpacesInParens: Never
SpacesInSquareBrackets: false
ReflowComments: Never
AlignEscapedNewlines: Left
LineEnding: DeriveLF
```

**不正确：** 函数左花括号同行、空函数合并、主动重排注释并强制转换为 LF。

```yaml
BreakBeforeBraces: Attach
AllowShortFunctionsOnASingleLine: Empty
ReflowComments: Always
LineEnding: LF
```

**可选正确：** 同样优先保留输入换行，新建且无换行可供推断的片段默认用 CRLF。
`DeriveLF` 与 `DeriveCRLF` 仅在无法推断时的默认值不同；已有输入仍按占多数的换行方式处理。
仅替换上面完整设置中的 `LineEnding`，其他设置不变。

```yaml
LineEnding: DeriveCRLF
```

### 8.2 静态分析按语言与规则配置

**应该**启用潜在错误、性能和可读性检查；现代化检查仅在适用语言下运行。
函数超过 80 行时提醒；关闭魔法数字检查不代表可以省略数值的语义说明。
参数数量和嵌套层级分别采用 5 和 4 的提醒阈值；圈复杂度目标通过支持该指标的分析工具或人工审查验证，
不把认知复杂度等其他指标当作同一个数值。
命名规则推荐自有代码使用小写，不强制枚举成员只能大写或只能小写，也不改写第三方与 SDK 命名。
有意贯穿时审查 `go through` 注释；不同工具对注释的识别不同，不能把注释当作通用的抑制机制。
要求显式贯穿属性的工具链，可采用已验证的属性或改写控制流；C99 代码不直接引入 C++ 属性。

**正确：** YAML 折叠字符串中的内容只有检查名称。

```yaml
Checks: >-
  -*,
  modernize-*,
  performance-*,
  bugprone-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers
WarningsAsErrors: 'bugprone-undefined-memory-manipulation,performance-no-automatic-move'
CheckOptions:
  readability-function-size.LineThreshold: '80'
  readability-function-size.ParameterThreshold: '5'
  readability-function-size.NestingThreshold: '4'
  modernize-loop-convert.MaxCopySize: '16'
```

**不正确：** `#` 位于字符串内部，成为检查名称的一部分。

```yaml
Checks: >-
  bugprone-*, # enable bug checks
  readability-*
```

**可选正确：** 仅分析 C 时，移除专用于 C++ 的检查组，保持 C99 要求；
若对应工具链要求贯穿属性，可以在经确认支持的 GCC／Clang 环境下使用下面的明确标记。

```yaml
Checks: '-*,bugprone-*,performance-*,readability-*,-readability-magic-numbers'
CheckOptions:
  readability-function-size.LineThreshold: '80'
  readability-function-size.ParameterThreshold: '5'
  readability-function-size.NestingThreshold: '4'
```

```c
switch (mode) {
    case 0:
        count += 1;
        /* go through */
        __attribute__((fallthrough));
    case 1:
        count += 2;
        break;
    default:
        break;
}
```

### 8.3 编辑器参数匹配构建目标

语言标准、包含路径、宏和目标架构**必须**匹配真实构建。路径条件使用正则表达式。
后台索引、补全和参数提示属于编辑器能力，不能替代编译与测试。

**正确：** 下面直接展示 C99 参数与有效的配置字段。

```yaml
If:
  PathMatch: .*\.c
CompileFlags:
  Add: [-std=c99, -Wall, -Wextra, -Wpedantic]
Index:
  Background: Build
Completion:
  AllScopes: true
Hover:
  ShowAKA: true
InlayHints:
  Enabled: true
  ParameterNames: true
  DeducedTypes: true
  Designators: true
```

**不正确：** 使用 glob 代替正则，并填写无效的语言标准字段。

```yaml
If:
  PathMatch: '**/*.c'
CompileFlags:
  Standard: c11
```

**可选正确：** 声明内容也确定只采用 C 语义时，可以扩大匹配范围；
需要同时供 C++ 解析的声明不强制附加 C 语言标准。

```yaml
If:
  PathMatch: .*\.(c|h)
CompileFlags:
  Add: [-std=c99]
```

### 8.4 编辑器保存行为统一

保存时**必须**应用 UTF-8、删除行尾空白和末尾换行；C/C++ 使用 4 空格。
LF 与 CRLF 都允许，默认不设置 `end_of_line`，保留已有文本的换行；新建文本按团队约定选择。
设置说明使用独立行，避免值后的行内注释产生解析歧义。

**正确：**

```ini
charset = utf-8
trim_trailing_whitespace = true
insert_final_newline = true
indent_style = space
indent_size = 4
```

**不正确：** 使用 Tab 和歧义注释，且对所有已有文本强制设置 LF，改写原有 CRLF 风格。

```ini
[*]
end_of_line = lf
indent_style = tab
indent_size = 4 # actually use two spaces
```

**可选正确：** 对明确约定的新建文本范围选择 LF 或 CRLF，下列两种取值择一使用，
不把它们作为改写第三方或既有文本的理由。

```ini
# Option A: use LF for the agreed scope.
end_of_line = lf
```

```ini
# Option B: use CRLF for the agreed scope.
end_of_line = crlf
```

### 8.5 检查覆盖行为，遵守修改范围

**应该**统一工具版本，升级后**必须**验证规则。自有修改进行排版、分析、编译和适用测试；
第三方库、官方／权威 SDK 与生成代码保留原有命名、换行、花括号及注释风格，
不附带无关的批量修改。排版和命名检查只选择自有代码，自有适配层采用本文规则。
同时审查全篇函数花括号、空函数、多语句宏续行符、贯穿标记及 off/on 配对。
测试应覆盖错误输入和边界，不仅检查正常路径。

**正确：**

```text
修改：有界缓冲区写入
检查：正常写入、空指针、零容量、容量恰好用尽、长度越界
验证：排版检查 → 静态分析 → 目标编译 → 对应测试
范围：本次涉及的自有代码
SDK：保留上游风格，仅在自有适配层统一接口
```

**不正确：**

```text
修改：有界缓冲区写入
检查：只调用一次正常写入
验证：编辑器没有红线即通过
范围：顺便重排所有供应商代码
SDK：把官方接口批量改名为自有小写命名
```

**可选正确：** 纯规范调整可以只验证文档和示例，并在排版规则改变时补充片段对照。

```text
修改：指针排版说明
检查：三类示例齐全，函数花括号、大小写选项、LF／CRLF、注释保护和贯穿标记一致
验证：示例格式、配置语法、编码、空白与公开链接格式
```

## 9. 公开参考资料

借鉴公开规范中的通用原则，并按 C99 与嵌入式场景选择适用规则。Google 的引用明确为 C++ 指南，
只借鉴声明自包含、所有权与可读性原则，不把 C++ 特性或命名体系当成 C 的要求。
Zephyr 的安全规则可用于检查求值顺序、大小与符号性；BARR-C 提供嵌入式缺陷预防的背景。
这些参考不覆盖本文已明确的花括号、大小写、换行和第三方代码约定。

- [C99 委员会草案及勘误汇总 N1256](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)
- [ClangFormat Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [Clang-Tidy 文档](https://clang.llvm.org/extra/clang-tidy/)
- [clangd 配置说明](https://clangd.llvm.org/config)
- [EditorConfig Specification](https://spec.editorconfig.org/)
- [Doxygen 文档命令](https://www.doxygen.nl/manual/commands.html)
- [RFC 2119：规范约束词](https://www.rfc-editor.org/rfc/rfc2119)
- [RFC 8174：约束词的明确用法](https://www.rfc-editor.org/rfc/rfc8174)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Zephyr Coding Guidelines](https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html)
- [BARR-C Embedded C Coding Standard](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard)
