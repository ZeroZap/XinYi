# XinYi Code Style Design Guide - Expanded Edition

## Overview

This guide consolidates all code style conventions from the XinYi project with extensive practical examples. It follows RFC2119/RFC8174 standards where **MUST**, **SHOULD**, **MAY**, and **OPTIONAL** have specific meanings.

---

## 1. General Rules with Examples

### 1.1 Language Standard & Spacing

**MUST use C99 standard, 4 spaces per indent, no tabs**

```c
/* ✓ Correct */
if (condition) {
    for (size_t i = 0; i < 10; ++i) {
        int32_t value = array[i];
        result = calculate(value, param1, param2);
    }
}

/* ✗ Wrong - tabs, inconsistent spacing */
if(condition){
	for(size_t i=0;i<10;++i){
		int32_t value=array[i];
		result=calculate(value,param1,param2);
	}
}
```

### 1.2 Naming Conventions

**Correct naming patterns:**
```c
/* Public functions - module_action_object */
void battery_get_voltage(uint16_t* voltage);
void battery_set_threshold(uint16_t threshold);
int32_t charger_calculate_current(void);

/* Private functions - prv_ prefix */
static void prv_initialize_adc(void);
static int32_t prv_read_sensor_raw(void);

/* Internal library functions - libname_int_ prefix */
static int32_t battery_int_calculate_soc(void);
static void battery_int_update_state(void);

/* Variables - lowercase with underscores */
uint32_t battery_voltage_mv;
int16_t temperature_celsius;
uint8_t is_charging;
```

---

## 2. Variables - Practical Examples

### 2.1 Declaration Order Pattern

```c
/**
 * \brief Calculate battery state of charge
 * \param[in] data: Battery measurement data
 * \param[out] state: Output state structure
 */
void battery_calculate_state(const battery_data_t* data, battery_state_t* state) {
    /* 1. Custom structures and pointers */
    battery_config_t config;
    battery_metrics_t* metrics;
    battery_history_t* history;

    /* 2. Integer types (wider unsigned first) */
    uint32_t voltage_mv;
    int32_t temperature_c;
    uint16_t capacity_mah;
    int16_t delta_temp;
    uint8_t status_flags;

    /* 3. Floating point */
    double soc_percent;
    float efficiency_ratio;

    /* Now executable statements */
    voltage_mv = data->voltage;
    temperature_c = data->temperature;

    if (voltage_mv > 0) {
        soc_percent = (voltage_mv * 100.0) / MAX_VOLTAGE;
    }
}
```

### 2.2 Pointer and Const Usage

```c
/* ✓ Correct pointer patterns */
void send_data(const void* data, size_t len);           /* Data not modified */
void modify_buffer(void* buffer, size_t len);           /* Data can be modified */
void set_callback(const void* const context);           /* Neither modified */

/* ✓ Correct const usage */
const battery_config_t* get_config(void);               /* Return immutable config */
void update_config(const battery_config_t* config);     /* Parameter immutable */

/* ✗ Wrong - unnecessary cast */
int* ptr = (int*)malloc(sizeof(int) * 10);              /* Wrong */
int* ptr = malloc(sizeof(*ptr) * 10);                   /* Correct */

/* ✓ Correct NULL comparison */
if (ptr != NULL) {
    *ptr = 42;
}

if (length > 0) {                                        /* Numeric comparison */
    process_data(buffer, length);
}

if (is_ready) {                                          /* Boolean check */
    start_operation();
}
```

### 2.3 Size and Length Variables

```c
/* ✓ Always use size_t for lengths */
void process_array(const int32_t* array, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        handle_element(array[i]);
    }
}

/* ✗ Wrong - using int for size */
void process_array_wrong(const int32_t* array, int count) {
    for (int i = 0; i < count; ++i) {
        handle_element(array[i]);
    }
}
```

---

## 3. Functions - Real-World Examples

### 3.1 Function Declaration and Documentation

```c
/**
 * \brief Get current battery voltage
 * \param[out] voltage: Pointer to store voltage in millivolts
 * \return 0 on success, negative error code otherwise
 * \note Voltage is updated from ADC measurement
 */
int32_t battery_get_voltage(uint16_t* voltage);

/**
 * \brief Set charging current limit
 * \param[in] current_ma: Current limit in milliamps
 * \return 0 on success, -1 if current exceeds maximum
 * \warning This function must be called before charging starts
 */
int32_t charger_set_current_limit(uint16_t current_ma);

/**
 * \brief Get battery state of charge
 * \return State of charge percentage (0-100), or -1 on error
 */
int32_t battery_get_soc(void);
```

### 3.2 Function Implementation Patterns

```c
/* ✓ Correct implementation */
int32_t battery_get_voltage(uint16_t* voltage) {
    int32_t result;
    uint16_t raw_value;

    if (voltage == NULL) {
        return -1;
    }

    result = adc_read_channel(ADC_BATTERY_CHANNEL, &raw_value);
    if (result == 0) {
        *voltage = raw_value;
    }

    return result;
}

/* ✓ Correct pointer return */
battery_config_t* battery_get_config(void) {
    static battery_config_t config = {
        .voltage_min = 2800,
        .voltage_max = 4200,
        .temperature_min = -20,
        .temperature_max = 60,
    };
    return &config;
}

/* ✓ Correct malloc usage */
battery_data_t* battery_allocate_data(size_t count) {
    battery_data_t* data;

    data = malloc(sizeof(*data) * count);
    if (data == NULL) {
        return NULL;
    }

    return data;
}
```

---

## 4. Structures and Typedefs - Patterns

### 4.1 Structure Declaration Patterns

```c
/* Pattern 1: Named structure only */
struct battery_config {
    uint16_t voltage_min;
    uint16_t voltage_max;
    int16_t temperature_min;
    int16_t temperature_max;
};

/* Pattern 2: Typedef only (anonymous) */
typedef struct {
    uint32_t voltage_mv;
    int16_t temperature_c;
    uint16_t capacity_mah;
    uint8_t status;
} battery_data_t;

/* Pattern 3: Named with typedef */
typedef struct battery_state {
    uint8_t charging;
    uint8_t discharging;
    uint8_t error;
    uint8_t reserved;
} battery_state_t;
```

### 4.2 Structure Initialization

```c
/* ✓ Correct C99 designated initializer */
battery_data_t data = {
    .voltage_mv = 4200,
    .temperature_c = 25,
    .capacity_mah = 3000,
    .status = 0,
};

/* ✓ Complex structure with trailing commas */
static const battery_config_t config = {
    .voltage_min = 2800,
    .voltage_max = 4200,
    .temperature_min = -20,
    .temperature_max = 60,
    .charge_current_ma = 2000,
    .discharge_current_ma = 3000,
};

/* ✗ Wrong - positional initialization */
battery_data_t data = {4200, 25, 3000, 0};

/* ✗ Wrong - mixed declarations */
typedef struct {
    int32_t a, b;  /* Wrong - should be separate lines */
} config_t;
```

### 4.3 Enumeration Patterns

```c
/* ✓ Correct enum with _t suffix */
typedef enum {
    BATTERY_STATE_IDLE = 0,
    BATTERY_STATE_CHARGING = 1,
    BATTERY_STATE_DISCHARGING = 2,
    BATTERY_STATE_ERROR = 3,
} battery_state_enum_t;

/* ✓ Enum with documentation */
typedef enum {
    CHARGER_MODE_OFF = 0,           /*!< Charger disabled */
    CHARGER_MODE_TRICKLE = 1,       /*!< Trickle charge mode */
    CHARGER_MODE_FAST = 2,          /*!< Fast charge mode */
    CHARGER_MODE_MAINTENANCE = 3,   /*!< Maintenance charge */
} charger_mode_t;

/* ✗ Wrong - mixed case */
typedef enum {
    MY_ENUM_TestA,
    my_enum_testb,
} my_enum_t;
```

---

## 5. Macros - Comprehensive Examples

### 5.1 Simple Value Macros

```c
/* ✓ Correct - all parameters and expression parenthesized */
#define XY_MIN(x, y)            ((x) < (y) ? (x) : (y))
#define XY_MAX(x, y)            ((x) > (y) ? (x) : (y))
#define XY_ABS(x)               ((x) < 0 ? -(x) : (x))
#define XY_CLAMP(v, min, max)   (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))

/* ✗ Wrong - missing parentheses */
#define MIN(x, y)               x < y ? x : y
#define SUM(x, y)               x + y
#define CLAMP(v, min, max)      v < min ? min : v > max ? max : v
```

### 5.2 Bit Operation Macros

```c
/* ✓ Correct bit macros */
#define XY_BIT(pos)             (1UL << (pos))
#define XY_BIT_SET(val, pos)    ((val) |= XY_BIT(pos))
#define XY_BIT_CLR(val, pos)    ((val) &= ~XY_BIT(pos))
#define XY_BIT_TST(val, pos)    (((val) & XY_BIT(pos)) != 0)
#define XY_BIT_MASK(start, len) (((1UL << (len)) - 1) << (start))

/* Usage examples */
uint32_t flags = 0;
XY_BIT_SET(flags, 5);           /* Set bit 5 */
if (XY_BIT_TST(flags, 5)) {     /* Test bit 5 */
    /* Bit is set */
}
XY_BIT_CLR(flags, 5);           /* Clear bit 5 */
```

### 5.3 Array and Container Macros

```c
/* ✓ Correct array size macro */
#define XY_ARRAY_SIZE(arr)      (sizeof(arr) / sizeof((arr)[0]))

/* Usage */
static const int32_t values[] = {1, 3, 5, 7, 9};
size_t count = XY_ARRAY_SIZE(values);  /* count == 5 */

/* ✓ Container of macro for embedded structures */
#define XY_CONTAINER_OF(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

/* Usage */
typedef struct {
    int32_t id;
    battery_data_t data;
} battery_record_t;

battery_data_t* data_ptr = &record->data;
battery_record_t* record = XY_CONTAINER_OF(data_ptr, battery_record_t, data);
```

### 5.4 Multi-Statement Macros

```c
/* ✓ Correct - do-while wrapper */
#define XY_SET_POINT(p, x, y) \
    do { \
        (p)->px = (x); \
        (p)->py = (y); \
    } while (0)

/* ✓ Unused parameter marker */
#define XY_UNUSED(x)            do { (void)(x); } while (0)

/* Usage */
void handler(int code) {
    XY_UNUSED(code);  /* Suppress unused parameter warning */
}

/* ✓ Assert-like macro */
#define XY_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            debug_print("Assertion failed: %s\n", #expr); \
            while (1) {} \
        } \
    } while (0)
```

### 5.5 Macro vs Inline Function

```c
/* ✓ Use macro for simple operations */
#define VOLTAGE_TO_PERCENT(v)   (((v) - 2800) * 100 / (4200 - 2800))

/* ✓ Use inline function for complex logic */
static inline int32_t calculate_soc(uint16_t voltage, int16_t temperature) {
    int32_t soc = (voltage - 2800) * 100 / (4200 - 2800);

    /* Temperature compensation */
    if (temperature < 0) {
        soc = (soc * 90) / 100;
    } else if (temperature > 50) {
        soc = (soc * 95) / 100;
    }

    return XY_CLAMP(soc, 0, 100);
}
```

---

## 6. Comments - Practical Examples

### 6.1 Comment Styles

```c
/* ✓ Single-line comment */
int32_t result = calculate();  /* Store calculation result */

/* ✓ Multi-line comment with space+asterisk */
/*
 * This function performs battery state calculation
 * considering voltage, temperature, and capacity
 * to determine accurate state of charge
 */

/* ✓ Member comment */
typedef struct {
    uint16_t voltage_mv;    /**< Voltage in millivolts */
    int16_t temperature_c;  /**< Temperature in Celsius */
    uint8_t status;         /**< Battery status flags */
} battery_data_t;

/* ✗ Wrong - C++ style comment */
// This is wrong style
int32_t value = 0;

/* ✗ Wrong - no space after asterisk */
/*
* This is wrong
* missing space
*/
```

### 6.2 Comment Alignment

```c
void battery_update(void) {
    uint32_t voltage;
    int16_t temperature;

    voltage = adc_read_voltage();           /* Read voltage from ADC */
    temperature = sensor_read_temperature(); /* Read temperature sensor */

    /* For very long statements, align to next 4-space indent */
    int32_t result = battery_calculate_complex_state_with_long_name(
        voltage, temperature, capacity);    /* Aligned to next indent */
}
```

---

## 7. Compound Statements - Examples

### 7.1 If/Else Statements

```c
/* ✓ Correct if-else structure */
if (voltage > 4200) {
    status = BATTERY_OVERVOLTAGE;
} else if (voltage < 2800) {
    status = BATTERY_UNDERVOLTAGE;
} else {
    status = BATTERY_OK;
}

/* ✓ Nested if with proper indentation */
if (is_charging) {
    if (temperature > 60) {
        reduce_charge_current();
    } else if (temperature < 0) {
        stop_charging();
    }
}

/* ✗ Wrong - no braces for single statement */
if (voltage > 4200)
    status = BATTERY_OVERVOLTAGE;
else
    status = BATTERY_OK;

/* ✗ Wrong - else on wrong line */
if (voltage > 4200) {
    status = BATTERY_OVERVOLTAGE;
}
else {
    status = BATTERY_OK;
}
```

### 7.2 Loop Patterns

```c
/* ✓ Preferred - for loop with counter in declaration */
for (size_t i = 0; i < count; ++i) {
    process_element(array[i]);
}

/* ✓ For loop when counter needed after loop */
size_t i;
for (i = 0; i < count; ++i) {
    if (array[i] == target) {
        break;
    }
}
if (i < count) {
    found_at_index(i);
}

/* ✓ While loop for condition-based iteration */
while (is_data_available()) {
    data_t item = read_data();
    process_data(&item);
}

/* ✓ Empty loop with single-line braces */
while (is_register_bit_set()) {}

/* ✗ Wrong - space in empty braces */
while (is_register_bit_set()) { }

/* ✗ Wrong - no braces for empty loop */
while (is_register_bit_set());
```

### 7.3 Ternary Operator

```c
/* ✓ Correct - assignment */
int32_t min_value = (a < b) ? a : b;

/* ✓ Correct - function parameter */
process_value((status == OK) ? value : default_value);

/* ✗ Wrong - function call without assignment */
(is_ready) ? start_operation() : stop_operation();

/* Better - use if-else for clarity */
if (is_ready) {
    start_operation();
} else {
    stop_operation();
}
```

---

## 8. Switch Statements - Examples

### 8.1 Basic Switch Structure

```c
/* ✓ Correct switch statement */
switch (battery_state) {
    case BATTERY_CHARGING:
        update_charge_indicator();
        break;
    case BATTERY_DISCHARGING:
        update_discharge_indicator();
        break;
    case BATTERY_FULL:
        stop_charging();
        break;
    default:
        handle_error();
        break;
}

/* ✓ Switch with local variables */
switch (mode) {
    case MODE_FAST: {
        int32_t current_ma = 2000;
        int16_t temp_limit = 50;
        set_charge_parameters(current_ma, temp_limit);
        break;
    }
    case MODE_SLOW: {
        int32_t current_ma = 500;
        int16_t temp_limit = 40;
        set_charge_parameters(current_ma, temp_limit);
        break;
    }
    default:
        break;
}

/* ✗ Wrong - case without indent */
switch (state) {
case 0:
    do_something();
    break;
case 1:
    do_other();
    break;
}

/* ✗ Wrong - break not indented */
switch (state) {
    case 0:
        do_something();
    break;  /* Wrong indent */
}
```

---

## 9. Documentation (Doxygen) - Examples

### 9.1 Function Documentation

```c
/**
 * \brief Calculate battery state of charge
 * \param[in] voltage: Battery voltage in millivolts
 * \param[in] temperature: Battery temperature in Celsius
 * \param[in] capacity: Battery capacity in milliamp-hours
 * \return State of charge percentage (0-100), or -1 on error
 * \note This function uses temperature compensation
 * \warning Voltage must be within valid range
 */
int32_t battery_calculate_soc(uint16_t voltage, int16_t temperature, uint16_t capacity);

/**
 * \brief Initialize battery management system
 * \param[in] config: Pointer to configuration structure
 * \return 0 on success, negative error code on failure
 * \retval -1 Invalid configuration
 * \retval -2 Hardware initialization failed
 */
int32_t battery_init(const battery_config_t* config);
```

### 9.2 Structure Documentation

```c
/**
 * \brief Battery measurement data
 */
typedef struct {
    uint16_t voltage_mv;    /**< Voltage in millivolts */
    int16_t temperature_c;  /**< Temperature in Celsius */
    uint16_t capacity_mah;  /**< Capacity in milliamp-hours */
    uint8_t status;         /**< Status flags */
} battery_data_t;

/**
 * \brief Battery state enumeration
 */
typedef enum {
    BATTERY_STATE_IDLE = 0,         /*!< Idle state */
    BATTERY_STATE_CHARGING = 1,     /*!< Charging in progress */
    BATTERY_STATE_DISCHARGING = 2,  /*!< Discharging in progress */
    BATTERY_STATE_ERROR = 3,        /*!< Error state */
} battery_state_t;
```

### 9.3 Macro Documentation

```c
/**
 * \brief Get minimum value between two numbers
 * \param[in] x: First value
 * \param[in] y: Second value
 * \return Minimum of x and y
 * \hideinitializer
 */
#define XY_MIN(x, y) ((x) < (y) ? (x) : (y))

/**
 * \brief Clamp value within range
 * \param[in] v: Value to clamp
 * \param[in] min: Minimum allowed value
 * \param[in] max: Maximum allowed value
 * \return Clamped value within [min, max]
 * \hideinitializer
 */
#define XY_CLAMP(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
```

---

## 10. Header and Source Files - Structure

### 10.1 Header File Template

```c
/**
 * \file            battery.h
 * \brief           Battery management module interface
 */

/*
 * Copyright (c) 2024 XinYi Project
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
 */

#ifndef BATTERY_H
#define BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Public type definitions */
typedef struct {
    uint16_t voltage_min;
    uint16_t voltage_max;
    int16_t temperature_min;
    int16_t temperature_max;
} battery_config_t;

/* Public function declarations */
int32_t battery_init(const battery_config_t* config);
int32_t battery_get_voltage(uint16_t* voltage);
int32_t battery_get_soc(void);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H */
```

### 10.2 Source File Template

```c
/**
 * \file            battery.c
 * \brief           Battery management module implementation
 */

#include "battery.h"
#include <stdlib.h>

/* Private type definitions */
typedef struct {
    uint16_t voltage;
    int16_t temperature;
    uint8_t status;
} battery_state_t;

/* Private variables */
static battery_state_t prv_state;
static battery_config_t prv_config;

/* Private function declarations */
static int32_t prv_read_adc(void);
static void prv_update_status(void);

/* Public function implementations */

/**
 * \brief Initialize battery module
 * \param[in] config: Configuration structure
 * \return 0 on success, -1 on error
 */
int32_t battery_init(const battery_config_t* config) {
    if (config == NULL) {
        return -1;
    }

    prv_config = *config;
    prv_state.status = 0;

    return 0;
}

/**
 * \brief Get battery voltage
 * \param[out] voltage: Pointer to voltage value
 * \return 0 on success, -1 on error
 */
int32_t battery_get_voltage(uint16_t* voltage) {
    if (voltage == NULL) {
        return -1;
    }

    *voltage = prv_state.voltage;
    return 0;
}

/* Private function implementations */

static int32_t prv_read_adc(void) {
    /* Implementation */
    return 0;
}

static void prv_update_status(void) {
    /* Implementation */
}
```

---

## 11. Conditional Compilation - Examples

### 11.1 Feature Flags

```c
/* ✓ Correct conditional compilation */
#if defined(BATTERY_ENABLE_LOGGING)
    #define BATTERY_LOG(fmt, ...) debug_printf(fmt, ##__VA_ARGS__)
#else /* defined(BATTERY_ENABLE_LOGGING) */
    #define BATTERY_LOG(fmt, ...) do { } while (0)
#endif /* !defined(BATTERY_ENABLE_LOGGING) */

/* ✓ Multiple conditions */
#if defined(BATTERY_ENABLE_FAST_CHARGE) && defined(BATTERY_ENABLE_THERMAL_CONTROL)
    static const uint16_t max_current = 3000;
#elif defined(BATTERY_ENABLE_FAST_CHARGE)
    static const uint16_t max_current = 2000;
#else /* defined(BATTERY_ENABLE_FAST_CHARGE) */
    static const uint16_t max_current = 1000;
#endif /* !defined(BATTERY_ENABLE_FAST_CHARGE) */

/* ✗ Wrong - using #ifdef instead of defined() */
#ifdef BATTERY_ENABLE_LOGGING
    #define BATTERY_LOG(fmt, ...) debug_printf(fmt, ##__VA_ARGS__)
#endif
```

### 11.2 Platform-Specific Code

```c
/* ✓ Correct platform selection */
#if defined(PLATFORM_STM32)
    #include "stm32_hal.h"
    #define ADC_CHANNEL 5
#elif defined(PLATFORM_NRF52)
    #include "nrf52_hal.h"
    #define ADC_CHANNEL 3
#else /* defined(PLATFORM_STM32) */
    #error "Unsupported platform"
#endif /* !defined(PLATFORM_STM32) */
```

---

## 12. Common Pitfalls - Quick Reference

| Issue | ✗ Wrong | ✓ Correct |
|-------|---------|----------|
| **Spacing** | `if(x)` | `if (x)` |
| **Function call** | `func (a, b)` | `func(a, b)` |
| **Operators** | `a=b+c` | `a = b + c` |
| **Comma spacing** | `func(a,b,c)` | `func(a, b, c)` |
| **Pointer alignment** | `char *ptr` | `char* ptr` |
| **Macro protection** | `#define MIN(x,y) x<y?x:y` | `#define MIN(x,y) ((x)<(y)?(x):(y))` |
| **Boolean compare** | `if (is_ok == 1)` | `if (is_ok)` |
| **Numeric compare** | `if (count)` | `if (count > 0)` |
| **Pointer compare** | `if (ptr)` | `if (ptr != NULL)` |
| **Malloc cast** | `int *p = (int*)malloc(...)` | `int *p = malloc(...)` |
| **VLA usage** | `int arr[size]` | `int *arr = malloc(sizeof(*arr)*size)` |
| **Global init** | `static int b = 4;` | Initialize in `init()` |
| **Comment style** | `// comment` | `/* comment */` |
| **Typedef suffix** | `struct name_t {}` | `typedef struct {} name_t;` |
| **Enum members** | `MY_ENUM_testA` | `MY_ENUM_TEST_A` |
| **Braces** | `if (x) do_a();` | `if (x) { do_a(); }` |
| **Else placement** | `}\nelse {` | `} else {` |
| **Empty loop** | `while (x) { }` | `while (x) {}` |
| **Conditional** | `#ifdef XYZ` | `#if defined(XYZ)` |
| **Indent in #if** | `#if defined(X)\n    #if defined(Y)` | `#if defined(X)\n#if defined(Y)` |

---

## 13. Quick Reference Checklist

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

## 14. References

- **RFC2119/RFC8174**: Keyword definitions (MUST, SHOULD, MAY, etc.)
- **C99 Standard**: Language specification
- **Doxygen**: Documentation generation tool
- **Clang-Format**: Code formatting tool
- **Project Files**: `docs/code_style/xy_code_style.md`, `xy_code_style.h`, `xy_code_style.c`
