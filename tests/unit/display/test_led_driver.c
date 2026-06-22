#include "unity.h"
#include "xy_led_driver.h"

static uint32_t g_last_color;
static uint16_t g_last_x;
static uint16_t g_last_y;
static int g_show_count;

static void fake_set_pixel(uint16_t x, uint16_t y, uint32_t color)
{
    g_last_x = x;
    g_last_y = y;
    g_last_color = color;
}

static uint32_t fake_get_pixel(uint16_t x, uint16_t y)
{
    return ((uint32_t)x << 16) | y;
}

static void fake_show(void)
{
    g_show_count++;
}

void setUp(void)
{
    g_last_color = 0U;
    g_last_x = 0U;
    g_last_y = 0U;
    g_show_count = 0;
}

void tearDown(void)
{
}

static xy_led_driver_t make_driver(void *user_data)
{
    xy_led_driver_t driver = {
        .width = 8,
        .height = 4,
        .bpp = 24,
        .set_pixel = fake_set_pixel,
        .get_pixel = fake_get_pixel,
        .show = fake_show,
        .user_data = user_data,
    };
    return driver;
}

static void test_led_gui_rejects_invalid_driver(void)
{
    TEST_ASSERT_EQUAL_INT(-1, xy_led_register_gui(NULL));

    xy_led_driver_t missing_callbacks = make_driver((void *)0x1234);
    missing_callbacks.set_pixel = NULL;
    TEST_ASSERT_EQUAL_INT(-1, xy_led_register_gui(&missing_callbacks));
    TEST_ASSERT_FALSE(xy_led_is_gui_enabled(&missing_callbacks));
    TEST_ASSERT_NULL(xy_led_get_gui_interface(&missing_callbacks));
}

static void test_led_gui_registers_display_without_overwriting_user_data(void)
{
    int backend_state = 42;
    xy_led_driver_t driver = make_driver(&backend_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    TEST_ASSERT_TRUE(xy_led_is_gui_enabled(&driver));
    TEST_ASSERT_EQUAL_PTR(&backend_state, driver.user_data);

    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);
    TEST_ASSERT_EQUAL_UINT16(8, display->width);
    TEST_ASSERT_EQUAL_UINT16(4, display->height);
    TEST_ASSERT_EQUAL_INT(XY_GUI_COLOR_RGB888, display->format);
    TEST_ASSERT_EQUAL_PTR(&driver, display->user_data);
    TEST_ASSERT_EQUAL_PTR(display, XY_LED_GET_GUI(&driver));

    display->set_pixel(2, 3, 0x112233U);
    TEST_ASSERT_EQUAL_UINT16(2, g_last_x);
    TEST_ASSERT_EQUAL_UINT16(3, g_last_y);
    TEST_ASSERT_EQUAL_UINT32(0x112233U, g_last_color);
    TEST_ASSERT_EQUAL_UINT32(0x00050006U, display->get_pixel(5, 6));

    display->fill_rect(1, 2, 2, 2, 0x445566U);
    TEST_ASSERT_EQUAL_UINT16(2, g_last_x);
    TEST_ASSERT_EQUAL_UINT16(3, g_last_y);
    TEST_ASSERT_EQUAL_UINT32(0x445566U, g_last_color);

    display->flush();
    TEST_ASSERT_EQUAL_INT(1, g_show_count);
}

static void test_led_gui_can_be_disabled_and_reenabled(void)
{
    int backend_state = 7;
    xy_led_driver_t driver = make_driver(&backend_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_led_enable_gui(&driver, false);
    TEST_ASSERT_FALSE(xy_led_is_gui_enabled(&driver));
    TEST_ASSERT_NULL(xy_led_get_gui_interface(&driver));
    TEST_ASSERT_NULL(XY_LED_GET_GUI(&driver));

    xy_led_enable_gui(&driver, true);
    TEST_ASSERT_TRUE(xy_led_is_gui_enabled(&driver));
    TEST_ASSERT_NOT_NULL(xy_led_get_gui_interface(&driver));
}

static void test_led_gui_uses_monochrome_format_for_one_bpp(void)
{
    int backend_state = 1;
    xy_led_driver_t driver = make_driver(&backend_state);
    driver.bpp = 1;

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);
    TEST_ASSERT_EQUAL_INT(XY_GUI_COLOR_MONO, display->format);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_gui_rejects_invalid_driver);
    RUN_TEST(test_led_gui_registers_display_without_overwriting_user_data);
    RUN_TEST(test_led_gui_can_be_disabled_and_reenabled);
    RUN_TEST(test_led_gui_uses_monochrome_format_for_one_bpp);
    return UNITY_END();
}
