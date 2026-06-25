#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_gui_event.h"
#include "xy_gui_button.h"
#include "xy_gui_checkbox.h"
#include "xy_gui_label.h"
#include "xy_gui_progress.h"
#include "xy_gui_slider.h"
#include "xy_gui_container.h"

uint32_t xy_os_tick_get(void) { return 1234; }

void xy_log_char(char ch)
{
    (void)ch;
}

static int g_cb_count;
static int g_cb_value;
static bool g_handler_seen;

void setUp(void)
{
}

void tearDown(void)
{
}

static void reset_cb(void)
{
    g_cb_count = 0;
    g_cb_value = 0;
    g_handler_seen = false;
}

static void widget_cb(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data)
{
    (void)widget;
    TEST_ASSERT_TRUE(user_data == (void *)0x1234 || user_data == (void *)0x5678);
    g_cb_count++;
    if (event) {
        g_cb_value = event->data.value;
    }
}

static bool listener_cb(xy_gui_event_t *event, void *user_data)
{
    TEST_ASSERT_EQUAL_PTR((void *)0xCAFE, user_data);
    TEST_ASSERT_NOT_NULL(event);
    g_handler_seen = true;
    event->handled = true;
    return true;
}

static void test_event_queue_and_dispatch(void)
{
    xy_gui_event_queue_t q;
    xy_gui_event_t event, out;
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_queue_init(&q));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_queue_count(&q));

    event = xy_gui_event_create_touch(XY_GUI_EVENT_TOUCH_DOWN, 10, 20);
    TEST_ASSERT_EQUAL_INT16(10, event.data.point.x);
    TEST_ASSERT_EQUAL_INT16(20, event.data.point.y);
    TEST_ASSERT_EQUAL_INT16(128, event.data.point.pressure);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_push(&q, &event));
    TEST_ASSERT_EQUAL_INT(1, xy_gui_event_queue_count(&q));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_pop(&q, &out));
    TEST_ASSERT_EQUAL_INT(XY_GUI_EVENT_TOUCH_DOWN, out.type);
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_event_pop(&q, &out));

    for (int i = 0; i < XY_GUI_EVENT_QUEUE_SIZE; ++i) {
        event = xy_gui_event_create_value_changed(i);
        TEST_ASSERT_EQUAL_INT(0, xy_gui_event_push(&q, &event));
    }
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_event_push(&q, &event));
    xy_gui_event_queue_clear(&q);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_queue_count(&q));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_system_init());
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_event_system_init());
    xy_gui_event_system_deinit();

    reset_cb();
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_register_listener(listener_cb, (void *)0xCAFE, 10));
    event = xy_gui_event_create_click(1, 2);
    TEST_ASSERT_TRUE(xy_gui_event_dispatch(&event));
    TEST_ASSERT_TRUE(g_handler_seen);
    TEST_ASSERT_TRUE(event.handled);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_event_unregister_listener(listener_cb));
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_event_unregister_listener(listener_cb));
}

static void test_button_contracts(void)
{
    xy_gui_button_t button;
    xy_gui_event_t event;
    uint16_t fb[80 * 40] = {0};

    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_create(&button, 2, 3, 60, 24, "Run", XY_GUI_BUTTON_NORMAL));
    TEST_ASSERT_EQUAL_STRING("Run", xy_gui_button_get_text(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_set_text(&button, "Go"));
    TEST_ASSERT_EQUAL_STRING("Go", xy_gui_button_get_text(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_set_pressed(&button, true));
    TEST_ASSERT_TRUE(xy_gui_button_is_pressed(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_set_checked(&button, true));
    TEST_ASSERT_TRUE(xy_gui_button_is_checked(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_set_icon(&button, (const uint8_t *)"x", 1, 1));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_draw(&button, fb, 80, 40));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_destroy(&button));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_create(&button, 0, 0, 40, 20, "T", XY_GUI_BUTTON_TOGGLE));
    reset_cb();
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_set_click_cb(&button, widget_cb, (void *)0x1234));
    TEST_ASSERT_FALSE(xy_gui_button_is_checked(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_trigger_click(&button));
    TEST_ASSERT_EQUAL_INT(1, g_cb_count);
    TEST_ASSERT_TRUE(xy_gui_button_is_checked(&button));

    event = xy_gui_event_create_touch(XY_GUI_EVENT_PRESS, 5, 5);
    event.timestamp = 100;
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_update(&button, &event));
    TEST_ASSERT_TRUE(xy_gui_button_is_pressed(&button));
    event = xy_gui_event_create_touch(XY_GUI_EVENT_RELEASE, 5, 5);
    event.timestamp = 120;
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_update(&button, &event));
    TEST_ASSERT_TRUE(event.handled);
    TEST_ASSERT_FALSE(xy_gui_button_is_pressed(&button));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_button_destroy(&button));
}

static void test_checkbox_contracts(void)
{
    xy_gui_checkbox_t checkbox, radio_a, radio_b;
    xy_gui_checkbox_group_t group;
    xy_gui_event_t event;
    uint16_t fb[100 * 40] = {0};

    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_create(&checkbox, 0, 0, "Opt", true));
    TEST_ASSERT_EQUAL_STRING("Opt", xy_gui_checkbox_get_text(&checkbox));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_set_checked(&checkbox, true));
    TEST_ASSERT_TRUE(xy_gui_checkbox_is_checked(&checkbox));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_toggle(&checkbox));
    TEST_ASSERT_EQUAL_INT(XY_GUI_CHECKBOX_INDETERMINATE, xy_gui_checkbox_get_state(&checkbox));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_set_state(&checkbox, XY_GUI_CHECKBOX_UNCHECKED));
    reset_cb();
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_set_state_changed_cb(&checkbox, widget_cb, (void *)0x1234));
    event = xy_gui_event_create_touch(XY_GUI_EVENT_RELEASE, 2, 2);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_update(&checkbox, &event));
    TEST_ASSERT_TRUE(event.handled);
    TEST_ASSERT_EQUAL_INT(1, g_cb_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_CHECKBOX_CHECKED, g_cb_value);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_draw(&checkbox, fb, 100, 40));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_destroy(&checkbox));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_radio_create(&radio_a, 0, 0, "A"));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_radio_create(&radio_b, 0, 20, "B"));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_group_init(&group));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_group_add(&group, &radio_a));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_group_add(&group, &radio_b));
    TEST_ASSERT_EQUAL_INT(2, group.count);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_group_set_selected(&group, 1));
    TEST_ASSERT_EQUAL_PTR(&radio_b, xy_gui_checkbox_group_get_selected(&group));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_group_set_selected(&group, 4));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_destroy(&radio_a));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_checkbox_destroy(&radio_b));
}

static void test_label_progress_slider_container(void)
{
    xy_gui_label_t label;
    xy_gui_progress_t progress;
    xy_gui_slider_t slider;
    xy_gui_container_t container;
    xy_gui_event_t event;
    uint16_t fb[160 * 80] = {0};

    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_create(&label, 1, 2, 0, 0, "Hi"));
    TEST_ASSERT_EQUAL_STRING("Hi", xy_gui_label_get_text(&label));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_set_text(&label, "Hello"));
    TEST_ASSERT_GREATER_THAN_INT(0, xy_gui_label_get_text_width(&label));
    TEST_ASSERT_GREATER_THAN_INT(0, xy_gui_label_get_text_height(&label));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_set_text_align(&label, XY_GUI_ALIGN_CENTER));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_set_word_wrap(&label, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_set_ellipsis(&label, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_draw(&label, fb, 160, 80));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_progress_create(&progress, 0, 0, 100, 10, 0, 100));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_progress_set_value(&progress, 150));
    TEST_ASSERT_EQUAL_INT(100, xy_gui_progress_get_value(&progress));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_progress_set_value(&progress, -10));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_progress_get_value(&progress));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_progress_set_type(&progress, XY_GUI_PROGRESS_INDETERMINATE));
    TEST_ASSERT_EQUAL_INT(0, progress.base.ops->draw(&progress.base, fb, 160, 80));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_create(&slider, 0, 20, 100, 0, 100, XY_GUI_SLIDER_HORIZONTAL));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_set_step(&slider, 10));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_set_value(&slider, 55));
    TEST_ASSERT_EQUAL_INT(50, xy_gui_slider_get_value(&slider));
    TEST_ASSERT_GREATER_THAN_INT(xy_gui_slider_value_to_pos(&slider, 0), xy_gui_slider_value_to_pos(&slider, 100));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(90, xy_gui_slider_pos_to_value(&slider, 999));
    reset_cb();
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_set_value_changed_cb(&slider, widget_cb, (void *)0x1234));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_set_continuous(&slider, true));
    event = xy_gui_event_create_touch(XY_GUI_EVENT_TOUCH_DOWN, 80, 30);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_update(&slider, &event));
    TEST_ASSERT_EQUAL_INT(1, g_cb_count);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_show_ticks(&slider, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_show_value(&slider, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_draw(&slider, fb, 160, 80));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_create(&container, 0, 0, 120, 70));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_set_auto_layout(&container, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_set_padding(&container, 3));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_set_spacing(&container, 2));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_add_child(&container, &label.base));
    TEST_ASSERT_EQUAL_INT(1, container.child_count);
    TEST_ASSERT_EQUAL_PTR(&container.base, label.base.parent);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_container_remove_child(&container, &label.base));
    TEST_ASSERT_EQUAL_INT(0, container.child_count);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_slider_destroy(&slider));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_label_destroy(&label));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_queue_and_dispatch);
    RUN_TEST(test_button_contracts);
    RUN_TEST(test_checkbox_contracts);
    RUN_TEST(test_label_progress_slider_container);
    return UNITY_END();
}
