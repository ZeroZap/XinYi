/**
 * @file test_pandora_platform_startup.c
 * @brief Host contract for the Pandora board-owned clock/power startup owner.
 *
 * `boards/pandora_stm32l475/pandora_platform_startup.c` is the reference board's
 * single board-local vendor clock owner: every Pandora image calls it right
 * after `xy_hal_sys_init()`, and it decides the HSE/PLL tree, the bus dividers,
 * the regulator voltage range and the Flash wait states. Before this target it
 * had no host coverage at all - only a clean board build.
 *
 * The fixture supplies the three whitelisted vendor calls plus the PWR clock
 * macro through the host-only `fake_vendor/stm32l4xx_hal.h` seam, records the
 * exact call order and payloads, and asserts the numeric invariants the real
 * STM32L4 datasheet imposes on that configuration. Assertions use the real
 * vendor constant values, so a silently changed divider or a swapped source
 * fails here instead of producing an overclocked board image.
 *
 * Evidence boundary: host/compile contract only. Nothing here proves HSE
 * start-up time, PLL lock, real 80 MHz SYSCLK, bus timing, or power behaviour
 * on hardware.
 */

#include "unity.h"

#include "stm32l4xx_hal.h"

#include "pandora_platform_startup.h"

#include <stdint.h>

/* Pandora's own stm32l4xx_hal_conf.h pins HSE_VALUE to 8 MHz. */
#define PANDORA_HSE_HZ 8000000U

/* STM32L4 (non-L4+/S) limits for this configuration. */
#define PLL_VCO_INPUT_MIN_HZ 4000000U
#define PLL_VCO_INPUT_MAX_HZ 16000000U
#define PLL_VCO_OUTPUT_MIN_HZ 64000000U
#define PLL_VCO_OUTPUT_MAX_HZ 344000000U
#define STM32L475_MAX_SYSCLK_HZ 80000000U
#define FLASH_LATENCY_80MHZ_VOS1 4U

/* --- vendor hook fixture ----------------------------------------------- */

typedef enum {
    SEQ_PWR_CLOCK = 1,
    SEQ_VOLTAGE_SCALING,
    SEQ_OSC_CONFIG,
    SEQ_CLOCK_CONFIG,
} hook_step_t;

static hook_step_t g_sequence[8];
static unsigned int g_sequence_len;

static unsigned int g_pwr_clock_calls;
static unsigned int g_vos_calls;
static uint32_t g_vos_arg;

static unsigned int g_osc_calls;
static RCC_OscInitTypeDef g_osc_capture;

static unsigned int g_clock_calls;
static RCC_ClkInitTypeDef g_clock_capture;
static uint32_t g_latency_capture;

static HAL_StatusTypeDef g_vos_result;
static HAL_StatusTypeDef g_osc_result;
static HAL_StatusTypeDef g_clock_result;

static void record_step(hook_step_t step) {
    if (g_sequence_len < (sizeof(g_sequence) / sizeof(g_sequence[0]))) {
        g_sequence[g_sequence_len] = step;
    }
    ++g_sequence_len;
}

void xy_test_fake_rcc_pwr_enable(void) {
    ++g_pwr_clock_calls;
    record_step(SEQ_PWR_CLOCK);
}

HAL_StatusTypeDef HAL_PWREx_ControlVoltageScaling(uint32_t VoltageScaling) {
    ++g_vos_calls;
    g_vos_arg = VoltageScaling;
    record_step(SEQ_VOLTAGE_SCALING);
    return g_vos_result;
}

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef* RCC_OscInitStruct) {
    ++g_osc_calls;
    if (RCC_OscInitStruct != NULL) {
        g_osc_capture = *RCC_OscInitStruct;
    }
    record_step(SEQ_OSC_CONFIG);
    return g_osc_result;
}

HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef* RCC_ClkInitStruct, uint32_t FLatency) {
    ++g_clock_calls;
    if (RCC_ClkInitStruct != NULL) {
        g_clock_capture = *RCC_ClkInitStruct;
    }
    g_latency_capture = FLatency;
    record_step(SEQ_CLOCK_CONFIG);
    return g_clock_result;
}

void setUp(void) {
    g_sequence_len = 0U;
    g_pwr_clock_calls = 0U;
    g_vos_calls = 0U;
    g_vos_arg = 0U;
    g_osc_calls = 0U;
    g_clock_calls = 0U;
    g_latency_capture = 0U;
    g_vos_result = HAL_OK;
    g_osc_result = HAL_OK;
    g_clock_result = HAL_OK;
    for (unsigned int i = 0U; i < (sizeof(g_sequence) / sizeof(g_sequence[0])); ++i) {
        g_sequence[i] = (hook_step_t)0;
    }
    g_osc_capture = (RCC_OscInitTypeDef){0};
    g_clock_capture = (RCC_ClkInitTypeDef){0};
}

void tearDown(void) {}

/* --- derived clock tree helpers ---------------------------------------- */

static uint32_t pll_vco_input_hz(void) {
    return PANDORA_HSE_HZ / g_osc_capture.PLL.PLLM;
}

static uint32_t pll_vco_output_hz(void) {
    return pll_vco_input_hz() * g_osc_capture.PLL.PLLN;
}

static uint32_t sysclk_hz(void) {
    return pll_vco_output_hz() / g_osc_capture.PLL.PLLR;
}

static uint32_t clk48_hz(void) {
    return pll_vco_output_hz() / g_osc_capture.PLL.PLLQ;
}

/* --- success path ------------------------------------------------------- */

static void test_startup_returns_zero_when_every_stage_succeeds(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());
}

static void test_startup_enables_pwr_clock_before_voltage_scaling(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(1U, g_pwr_clock_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_vos_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_osc_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_clock_calls);
}

static void test_startup_configures_the_tree_in_escalating_order(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(4U, g_sequence_len);
    TEST_ASSERT_EQUAL_INT(SEQ_PWR_CLOCK, g_sequence[0]);
    TEST_ASSERT_EQUAL_INT(SEQ_VOLTAGE_SCALING, g_sequence[1]);
    TEST_ASSERT_EQUAL_INT(SEQ_OSC_CONFIG, g_sequence[2]);
    TEST_ASSERT_EQUAL_INT(SEQ_CLOCK_CONFIG, g_sequence[3]);
}

static void test_startup_requests_voltage_scale1_before_raising_the_clock(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(PWR_REGULATOR_VOLTAGE_SCALE1, g_vos_arg);
}

static void test_startup_reissues_the_identical_configuration_on_a_second_call(void) {
    RCC_OscInitTypeDef first_osc;
    RCC_ClkInitTypeDef first_clock;

    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());
    first_osc = g_osc_capture;
    first_clock = g_clock_capture;

    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(2U, g_pwr_clock_calls);
    TEST_ASSERT_EQUAL_UINT(2U, g_osc_calls);
    TEST_ASSERT_EQUAL_UINT(2U, g_clock_calls);
    TEST_ASSERT_EQUAL_MEMORY(&first_osc, &g_osc_capture, sizeof(first_osc));
    TEST_ASSERT_EQUAL_MEMORY(&first_clock, &g_clock_capture, sizeof(first_clock));
}

/* --- failure propagation ----------------------------------------------- */

static void test_startup_fails_when_voltage_scaling_fails(void) {
    g_vos_result = HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());
}

static void test_failed_voltage_scaling_leaves_the_oscillator_untouched(void) {
    g_vos_result = HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(1U, g_pwr_clock_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_vos_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_osc_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_clock_calls);
}

static void test_startup_fails_when_the_oscillator_configuration_fails(void) {
    g_osc_result = HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());
}

static void test_failed_oscillator_configuration_never_switches_the_sysclk(void) {
    g_osc_result = HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(1U, g_vos_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_osc_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_clock_calls);
}

static void test_startup_fails_when_the_clock_switch_fails(void) {
    g_clock_result = HAL_ERROR;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());
}

static void test_failed_clock_switch_reports_failure_after_configuring_the_pll(void) {
    g_clock_result = HAL_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(-1, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT(1U, g_osc_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_clock_calls);
}

/* --- oscillator payload ------------------------------------------------- */

static void test_osc_config_enables_only_hse(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(RCC_OSCILLATORTYPE_HSE, g_osc_capture.OscillatorType);
    TEST_ASSERT_EQUAL_HEX32(RCC_HSE_ON, g_osc_capture.HSEState);
}

static void test_osc_config_leaves_every_other_oscillator_off(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.HSIState);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.HSICalibrationValue);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.LSEState);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.LSIState);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.MSIState);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.MSICalibrationValue);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.MSIClockRange);
    TEST_ASSERT_EQUAL_HEX32(0U, g_osc_capture.HSI48State);
}

static void test_osc_config_drives_the_pll_from_hse(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(RCC_PLL_ON, g_osc_capture.PLL.PLLState);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLSOURCE_HSE, g_osc_capture.PLL.PLLSource);
}

static void test_osc_config_uses_the_documented_pll_factors(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT32(1U, g_osc_capture.PLL.PLLM);
    TEST_ASSERT_EQUAL_UINT32(20U, g_osc_capture.PLL.PLLN);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLP_DIV7, g_osc_capture.PLL.PLLP);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLQ_DIV2, g_osc_capture.PLL.PLLQ);
    TEST_ASSERT_EQUAL_HEX32(RCC_PLLR_DIV2, g_osc_capture.PLL.PLLR);
}

/* --- clock stick payload ------------------------------------------------ */

static void test_clock_config_covers_every_bus_clock(void) {
    const uint32_t expected =
        RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;

    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(expected, g_clock_capture.ClockType);
}

static void test_clock_config_selects_the_pll_as_sysclk(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(RCC_SYSCLKSOURCE_PLLCLK, g_clock_capture.SYSCLKSource);
}

static void test_clock_config_runs_every_bus_undivided(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(RCC_SYSCLK_DIV1, g_clock_capture.AHBCLKDivider);
    TEST_ASSERT_EQUAL_HEX32(RCC_HCLK_DIV1, g_clock_capture.APB1CLKDivider);
    TEST_ASSERT_EQUAL_HEX32(RCC_HCLK_DIV1, g_clock_capture.APB2CLKDivider);
}

static void test_clock_config_requests_four_flash_wait_states(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_HEX32(FLASH_LATENCY_4, g_latency_capture);
    TEST_ASSERT_EQUAL_UINT32(FLASH_LATENCY_80MHZ_VOS1, g_latency_capture);
}

/* --- datasheet-derived invariants -------------------------------------- */

static void test_pll_factors_stay_inside_the_vendor_field_ranges(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_GREATER_OR_EQUAL(1, (int)g_osc_capture.PLL.PLLM);
    TEST_ASSERT_LESS_OR_EQUAL(8, (int)g_osc_capture.PLL.PLLM);
    TEST_ASSERT_GREATER_OR_EQUAL(8, (int)g_osc_capture.PLL.PLLN);
    TEST_ASSERT_LESS_OR_EQUAL(86, (int)g_osc_capture.PLL.PLLN);
}

static void test_pll_vco_input_stays_inside_the_lockable_range(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_GREATER_OR_EQUAL((int)PLL_VCO_INPUT_MIN_HZ, (int)pll_vco_input_hz());
    TEST_ASSERT_LESS_OR_EQUAL((int)PLL_VCO_INPUT_MAX_HZ, (int)pll_vco_input_hz());
}

static void test_pll_vco_output_stays_inside_the_lockable_range(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_GREATER_OR_EQUAL((int)PLL_VCO_OUTPUT_MIN_HZ, (int)pll_vco_output_hz());
    TEST_ASSERT_LESS_OR_EQUAL((int)PLL_VCO_OUTPUT_MAX_HZ, (int)pll_vco_output_hz());
}

static void test_the_tree_runs_the_board_at_its_80mhz_sysclk(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    /* 8 MHz HSE / PLLM 1 * PLLN 20 / PLLR 2 = 80 MHz. */
    TEST_ASSERT_EQUAL_UINT32(STM32L475_MAX_SYSCLK_HZ, sysclk_hz());
}

static void test_the_dividers_keep_every_bus_at_or_below_sysclk(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    /* RCC_*DIV1 vendor encodings are all zero, i.e. "not divided". */
    TEST_ASSERT_EQUAL_HEX32(0U, RCC_SYSCLK_DIV1);
    TEST_ASSERT_EQUAL_HEX32(0U, RCC_HCLK_DIV1);
    TEST_ASSERT_EQUAL_HEX32(RCC_SYSCLK_DIV1, g_clock_capture.AHBCLKDivider);
    TEST_ASSERT_EQUAL_HEX32(RCC_HCLK_DIV1, g_clock_capture.APB1CLKDivider);
    TEST_ASSERT_EQUAL_HEX32(RCC_HCLK_DIV1, g_clock_capture.APB2CLKDivider);
}

/**
 * Characterisation, not endorsement: PLLQ = /2 feeds the CLK48 domain with
 * 80 MHz. SDMMC1 / RNG / USB require exactly 48 MHz there, and no current
 * Pandora image selects a CLK48 consumer or changes CLK48SEL away from the
 * default PLLQ. Enabling one of them later needs its own clock source, not
 * this PLL.
 */
static void test_pllq_would_feed_clk48_at_80mhz(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_platform_startup());

    TEST_ASSERT_EQUAL_UINT32(80000000U, clk48_hz());
    TEST_ASSERT_NOT_EQUAL(48000000U, clk48_hz());
}

/* --- runner ------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_startup_returns_zero_when_every_stage_succeeds);
    RUN_TEST(test_startup_enables_pwr_clock_before_voltage_scaling);
    RUN_TEST(test_startup_configures_the_tree_in_escalating_order);
    RUN_TEST(test_startup_requests_voltage_scale1_before_raising_the_clock);
    RUN_TEST(test_startup_reissues_the_identical_configuration_on_a_second_call);
    RUN_TEST(test_startup_fails_when_voltage_scaling_fails);
    RUN_TEST(test_failed_voltage_scaling_leaves_the_oscillator_untouched);
    RUN_TEST(test_startup_fails_when_the_oscillator_configuration_fails);
    RUN_TEST(test_failed_oscillator_configuration_never_switches_the_sysclk);
    RUN_TEST(test_startup_fails_when_the_clock_switch_fails);
    RUN_TEST(test_failed_clock_switch_reports_failure_after_configuring_the_pll);
    RUN_TEST(test_osc_config_enables_only_hse);
    RUN_TEST(test_osc_config_leaves_every_other_oscillator_off);
    RUN_TEST(test_osc_config_drives_the_pll_from_hse);
    RUN_TEST(test_osc_config_uses_the_documented_pll_factors);
    RUN_TEST(test_clock_config_covers_every_bus_clock);
    RUN_TEST(test_clock_config_selects_the_pll_as_sysclk);
    RUN_TEST(test_clock_config_runs_every_bus_undivided);
    RUN_TEST(test_clock_config_requests_four_flash_wait_states);
    RUN_TEST(test_pll_factors_stay_inside_the_vendor_field_ranges);
    RUN_TEST(test_pll_vco_input_stays_inside_the_lockable_range);
    RUN_TEST(test_pll_vco_output_stays_inside_the_lockable_range);
    RUN_TEST(test_the_tree_runs_the_board_at_its_80mhz_sysclk);
    RUN_TEST(test_the_dividers_keep_every_bus_at_or_below_sysclk);
    RUN_TEST(test_pllq_would_feed_clk48_at_80mhz);
    return UNITY_END();
}
