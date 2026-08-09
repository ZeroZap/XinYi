#include "unity.h"
#include "fff.h"
#include "xy_gui_display.h"
#include "xy_led_driver.h"

static uint32_t g_last_color;
static uint16_t g_last_x;
static uint16_t g_last_y;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(fake_set_pixel, uint16_t, uint16_t, uint32_t)
FAKE_VOID_FUNC(fake_alt_set_pixel, uint16_t, uint16_t, uint32_t)
FAKE_VALUE_FUNC(uint32_t, fake_get_pixel, uint16_t, uint16_t)
FAKE_VALUE_FUNC(uint32_t, fake_alt_get_pixel, uint16_t, uint16_t)
FAKE_VOID_FUNC(fake_show)
FAKE_VOID_FUNC(fake_alt_show)

static void fake_set_pixel_impl(uint16_t x, uint16_t y, uint32_t color)
{
    g_last_x = x;
    g_last_y = y;
    g_last_color = color;
}

static uint32_t fake_get_pixel_impl(uint16_t x, uint16_t y)
{
    return ((uint32_t)x << 16) | y;
}

void setUp(void)
{
    RESET_FAKE(fake_set_pixel);
    RESET_FAKE(fake_alt_set_pixel);
    RESET_FAKE(fake_get_pixel);
    RESET_FAKE(fake_alt_get_pixel);
    RESET_FAKE(fake_show);
    RESET_FAKE(fake_alt_show);
    FFF_RESET_HISTORY();

    fake_set_pixel_fake.custom_fake = fake_set_pixel_impl;
    fake_get_pixel_fake.custom_fake = fake_get_pixel_impl;

    g_last_color = 0U;
    g_last_x = 0U;
    g_last_y = 0U;
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

static xy_led_driver_t make_alt_driver(void *user_data)
{
    xy_led_driver_t driver = {
        .width = 8,
        .height = 4,
        .bpp = 24,
        .set_pixel = fake_alt_set_pixel,
        .get_pixel = fake_alt_get_pixel,
        .show = fake_alt_show,
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
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(2, fake_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(3, fake_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0x112233U, fake_set_pixel_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT16(2, g_last_x);
    TEST_ASSERT_EQUAL_UINT16(3, g_last_y);
    TEST_ASSERT_EQUAL_UINT32(0x112233U, g_last_color);
    TEST_ASSERT_EQUAL_UINT32(0x00050003U, display->get_pixel(5, 3));
    TEST_ASSERT_EQUAL_UINT(1U, fake_get_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(5, fake_get_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(3, fake_get_pixel_fake.arg1_val);

    display->fill_rect(1, 2, 2, 2, 0x445566U);
    TEST_ASSERT_EQUAL_UINT(5U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(2, g_last_x);
    TEST_ASSERT_EQUAL_UINT16(3, g_last_y);
    TEST_ASSERT_EQUAL_UINT32(0x445566U, g_last_color);

    display->flush();
    TEST_ASSERT_EQUAL_UINT(1U, fake_show_fake.call_count);
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

static void test_led_gui_disabled_display_pointer_has_no_backend_side_effects(void)
{
    int backend_state = 9;
    xy_led_driver_t driver = make_driver(&backend_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);

    xy_led_enable_gui(&driver, false);
    TEST_ASSERT_FALSE(xy_led_is_gui_enabled(&driver));
    TEST_ASSERT_NULL(xy_led_get_gui_interface(&driver));

    display->set_pixel(1, 1, 0x112233U);
    TEST_ASSERT_EQUAL_UINT32(0U, display->get_pixel(1, 1));
    display->fill_rect(0, 0, 2, 2, 0x445566U);
    display->flush();

    TEST_ASSERT_EQUAL_UINT(0U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, fake_get_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, fake_show_fake.call_count);
}

static void test_led_gui_display_callbacks_remain_bound_to_their_registered_driver(void)
{
    int first_state = 11;
    int second_state = 22;
    xy_led_driver_t first = make_driver(&first_state);
    xy_led_driver_t second = make_alt_driver(&second_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&first));
    xy_gui_display_t *first_display = xy_led_get_gui_interface(&first);
    TEST_ASSERT_NOT_NULL(first_display);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&second));
    xy_gui_display_t *second_display = xy_led_get_gui_interface(&second);
    TEST_ASSERT_NOT_NULL(second_display);

    first_display->set_pixel(1, 2, 0x010203U);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, fake_alt_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(1, fake_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(2, fake_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0x010203U, fake_set_pixel_fake.arg2_val);

    second_display->set_pixel(3, 1, 0x0A0B0CU);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, fake_alt_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(3, fake_alt_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(1, fake_alt_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0x0A0B0CU, fake_alt_set_pixel_fake.arg2_val);
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

static void test_led_gui_callbacks_ignore_invalid_geometry_without_backend_side_effects(void)
{
    int backend_state = 3;
    xy_led_driver_t driver = make_driver(&backend_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);

    display->set_pixel(-1, 0, 0x010203U);
    display->set_pixel(0, -1, 0x040506U);
    TEST_ASSERT_EQUAL_UINT(0U, fake_set_pixel_fake.call_count);

    TEST_ASSERT_EQUAL_UINT32(0U, display->get_pixel(-1, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, display->get_pixel(0, -1));
    TEST_ASSERT_EQUAL_UINT(0U, fake_get_pixel_fake.call_count);

    display->fill_rect(1, 1, 0, 2, 0x112233U);
    display->fill_rect(1, 1, 2, 0, 0x445566U);
    TEST_ASSERT_EQUAL_UINT(0U, fake_set_pixel_fake.call_count);

    display->fill_rect(-1, -1, 2, 2, 0x778899U);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(0, fake_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(0, fake_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0x778899U, fake_set_pixel_fake.arg2_val);

    display->set_pixel(8, 0, 0x010203U);
    display->set_pixel(0, 4, 0x040506U);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);

    TEST_ASSERT_EQUAL_UINT32(0U, display->get_pixel(8, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, display->get_pixel(0, 4));
    TEST_ASSERT_EQUAL_UINT(0U, fake_get_pixel_fake.call_count);

    display->fill_rect(7, 3, 3, 3, 0xAABBCCU);
    TEST_ASSERT_EQUAL_UINT(2U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(7, fake_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(3, fake_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0xAABBCCU, fake_set_pixel_fake.arg2_val);
}

static void test_led_gui_inline_display_helpers_forward_to_registered_backend(void)
{
    int backend_state = 5;
    xy_led_driver_t driver = make_driver(&backend_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);

    xy_gui_display_set_pixel(display, 2, 1, 0x00ABCDEFU);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(2, fake_set_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(1, fake_set_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0x00ABCDEFU, fake_set_pixel_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT16(2, g_last_x);
    TEST_ASSERT_EQUAL_UINT16(1, g_last_y);
    TEST_ASSERT_EQUAL_UINT32(0x00ABCDEFU, g_last_color);

    TEST_ASSERT_EQUAL_UINT32(0x00030002U, xy_gui_display_get_pixel(display, 3, 2));
    TEST_ASSERT_EQUAL_UINT(1U, fake_get_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(3, fake_get_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(2, fake_get_pixel_fake.arg1_val);

    xy_gui_display_flush(display);
    TEST_ASSERT_EQUAL_UINT(1U, fake_show_fake.call_count);

    xy_gui_display_set_pixel(NULL, 1, 1, 0x111111U);
    TEST_ASSERT_EQUAL_UINT(1U, fake_set_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_gui_display_get_pixel(NULL, 1, 1));
    xy_gui_display_flush(NULL);
    TEST_ASSERT_EQUAL_UINT(1U, fake_show_fake.call_count);
}

static void test_led_gui_reregister_refreshes_optional_callbacks_and_metadata(void)
{
    int first_state = 5;
    int second_state = 6;
    xy_led_driver_t driver = make_driver(&first_state);

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));
    xy_gui_display_t *display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_NOT_NULL(display);
    TEST_ASSERT_NOT_NULL(display->get_pixel);
    TEST_ASSERT_EQUAL_PTR(&driver, display->user_data);
    TEST_ASSERT_EQUAL_INT(XY_GUI_COLOR_RGB888, display->format);

    driver.user_data = &second_state;
    driver.get_pixel = NULL;
    driver.bpp = 1;
    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&driver));

    xy_gui_display_t *updated_display = xy_led_get_gui_interface(&driver);
    TEST_ASSERT_EQUAL_PTR(display, updated_display);
    TEST_ASSERT_EQUAL_PTR(&driver, updated_display->user_data);
    TEST_ASSERT_NULL(updated_display->get_pixel);
    TEST_ASSERT_EQUAL_INT(XY_GUI_COLOR_MONO, updated_display->format);
    TEST_ASSERT_EQUAL_PTR(&second_state, driver.user_data);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_gui_rejects_invalid_driver);
    RUN_TEST(test_led_gui_registers_display_without_overwriting_user_data);
    RUN_TEST(test_led_gui_can_be_disabled_and_reenabled);
    RUN_TEST(test_led_gui_disabled_display_pointer_has_no_backend_side_effects);
    RUN_TEST(test_led_gui_display_callbacks_remain_bound_to_their_registered_driver);
    RUN_TEST(test_led_gui_uses_monochrome_format_for_one_bpp);
    RUN_TEST(test_led_gui_callbacks_ignore_invalid_geometry_without_backend_side_effects);
    RUN_TEST(test_led_gui_inline_display_helpers_forward_to_registered_backend);
    RUN_TEST(test_led_gui_reregister_refreshes_optional_callbacks_and_metadata);
    return UNITY_END();
}
