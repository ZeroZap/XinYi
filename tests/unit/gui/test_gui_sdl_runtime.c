#include "unity.h"
#include "xy_gui_sdl.h"

#include <SDL.h>

void setUp(void)
{
    xy_gui_sdl_deinit();
}

void tearDown(void)
{
    xy_gui_sdl_deinit();
}

static void test_dummy_video_driver_runs_real_sdl_lifecycle(void)
{
    xy_gui_disp_drv_t *driver = xy_gui_sdl_get_driver();

    TEST_ASSERT_EQUAL_INT(0, SDL_setenv("SDL_VIDEODRIVER", "dummy", 1));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_sdl_configure(8U, 8U, 1U, "XinYi SDL runtime"));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->init());
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->fill_rect(0, 0, 8, 8, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->flush());
    TEST_ASSERT_TRUE(xy_gui_sdl_poll_events());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dummy_video_driver_runs_real_sdl_lifecycle);
    return UNITY_END();
}