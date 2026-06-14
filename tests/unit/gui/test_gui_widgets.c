#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

static void reset_cb(void)
{
    g_cb_count = 0;
    g_cb_value = 0;
    g_handler_seen = false;
}

static void widget_cb(xy_gui_widget_t *widget, xy_gui_event_t *event, void *user_data)
{
    (void)widget;
    assert(user_data == (void*)0x1234 || user_data == (void*)0x5678);
    g_cb_count++;
    if (event) {
        g_cb_value = event->data.value;
    }
}

static bool listener_cb(xy_gui_event_t *event, void *user_data)
{
    assert(user_data == (void*)0xCAFE);
    assert(event != NULL);
    g_handler_seen = true;
    event->handled = true;
    return true;
}

static void test_event_queue_and_dispatch(void)
{
    xy_gui_event_queue_t q;
    xy_gui_event_t e, out;
    assert(xy_gui_event_queue_init(&q) == 0);
    assert(xy_gui_event_queue_count(&q) == 0);

    e = xy_gui_event_create_touch(XY_GUI_EVENT_TOUCH_DOWN, 10, 20);
    assert(e.data.point.x == 10 && e.data.point.y == 20);
    assert(e.data.point.pressure == 128);
    assert(xy_gui_event_push(&q, &e) == 0);
    assert(xy_gui_event_queue_count(&q) == 1);
    assert(xy_gui_event_pop(&q, &out) == 0);
    assert(out.type == XY_GUI_EVENT_TOUCH_DOWN);
    assert(xy_gui_event_pop(&q, &out) == -1);

    for (int i = 0; i < XY_GUI_EVENT_QUEUE_SIZE; ++i) {
        e = xy_gui_event_create_value_changed(i);
        assert(xy_gui_event_push(&q, &e) == 0);
    }
    assert(xy_gui_event_push(&q, &e) == -1);
    xy_gui_event_queue_clear(&q);
    assert(xy_gui_event_queue_count(&q) == 0);

    assert(xy_gui_event_system_init() == 0);
    assert(xy_gui_event_system_init() == -1);
    xy_gui_event_system_deinit();

    reset_cb();
    assert(xy_gui_event_register_listener(listener_cb, (void*)0xCAFE, 10) == 0);
    e = xy_gui_event_create_click(1, 2);
    assert(xy_gui_event_dispatch(&e) == true);
    assert(g_handler_seen && e.handled);
    assert(xy_gui_event_unregister_listener(listener_cb) == 0);
    assert(xy_gui_event_unregister_listener(listener_cb) == -1);
}

static void test_button_contracts(void)
{
    xy_gui_button_t b;
    xy_gui_event_t ev;
    uint16_t fb[80 * 40] = {0};

    assert(xy_gui_button_create(&b, 2, 3, 60, 24, "Run", XY_GUI_BUTTON_NORMAL) == 0);
    assert(strcmp(xy_gui_button_get_text(&b), "Run") == 0);
    assert(xy_gui_button_set_text(&b, "Go") == 0);
    assert(strcmp(xy_gui_button_get_text(&b), "Go") == 0);
    assert(xy_gui_button_set_pressed(&b, true) == 0);
    assert(xy_gui_button_is_pressed(&b));
    assert(xy_gui_button_set_checked(&b, true) == 0);
    assert(xy_gui_button_is_checked(&b));
    assert(xy_gui_button_set_icon(&b, (const uint8_t*)"x", 1, 1) == 0);
    assert(xy_gui_button_draw(&b, fb, 80, 40) == 0);
    assert(xy_gui_button_destroy(&b) == 0);

    assert(xy_gui_button_create(&b, 0, 0, 40, 20, "T", XY_GUI_BUTTON_TOGGLE) == 0);
    reset_cb();
    assert(xy_gui_button_set_click_cb(&b, widget_cb, (void*)0x1234) == 0);
    assert(!xy_gui_button_is_checked(&b));
    assert(xy_gui_button_trigger_click(&b) == 0);
    assert(g_cb_count == 1 && xy_gui_button_is_checked(&b));

    ev = xy_gui_event_create_touch(XY_GUI_EVENT_PRESS, 5, 5);
    ev.timestamp = 100;
    assert(xy_gui_button_update(&b, &ev) == 0);
    assert(xy_gui_button_is_pressed(&b));
    ev = xy_gui_event_create_touch(XY_GUI_EVENT_RELEASE, 5, 5);
    ev.timestamp = 120;
    assert(xy_gui_button_update(&b, &ev) == 0);
    assert(ev.handled && !xy_gui_button_is_pressed(&b));
    assert(xy_gui_button_destroy(&b) == 0);
}

static void test_checkbox_contracts(void)
{
    xy_gui_checkbox_t c, a, b;
    xy_gui_checkbox_group_t group;
    xy_gui_event_t ev;
    uint16_t fb[100 * 40] = {0};

    assert(xy_gui_checkbox_create(&c, 0, 0, "Opt", true) == 0);
    assert(strcmp(xy_gui_checkbox_get_text(&c), "Opt") == 0);
    assert(xy_gui_checkbox_set_checked(&c, true) == 0);
    assert(xy_gui_checkbox_is_checked(&c));
    assert(xy_gui_checkbox_toggle(&c) == 0);
    assert(xy_gui_checkbox_get_state(&c) == XY_GUI_CHECKBOX_INDETERMINATE);
    assert(xy_gui_checkbox_set_state(&c, XY_GUI_CHECKBOX_UNCHECKED) == 0);
    reset_cb();
    assert(xy_gui_checkbox_set_state_changed_cb(&c, widget_cb, (void*)0x1234) == 0);
    ev = xy_gui_event_create_touch(XY_GUI_EVENT_RELEASE, 2, 2);
    assert(xy_gui_checkbox_update(&c, &ev) == 0);
    assert(ev.handled && g_cb_count == 1 && g_cb_value == XY_GUI_CHECKBOX_CHECKED);
    assert(xy_gui_checkbox_draw(&c, fb, 100, 40) == 0);
    assert(xy_gui_checkbox_destroy(&c) == 0);

    assert(xy_gui_radio_create(&a, 0, 0, "A") == 0);
    assert(xy_gui_radio_create(&b, 0, 20, "B") == 0);
    assert(xy_gui_checkbox_group_init(&group) == 0);
    assert(xy_gui_checkbox_group_add(&group, &a) == 0);
    assert(xy_gui_checkbox_group_add(&group, &b) == 0);
    assert(group.count == 2);
    assert(xy_gui_checkbox_group_set_selected(&group, 1) == 0);
    assert(xy_gui_checkbox_group_get_selected(&group) == &b);
    assert(xy_gui_checkbox_group_set_selected(&group, 4) == 0);
    assert(xy_gui_checkbox_destroy(&a) == 0);
    assert(xy_gui_checkbox_destroy(&b) == 0);
}

static void test_label_progress_slider_container(void)
{
    xy_gui_label_t label;
    xy_gui_progress_t progress;
    xy_gui_slider_t slider;
    xy_gui_container_t container;
    xy_gui_event_t ev;
    uint16_t fb[160 * 80] = {0};

    assert(xy_gui_label_create(&label, 1, 2, 0, 0, "Hi") == 0);
    assert(strcmp(xy_gui_label_get_text(&label), "Hi") == 0);
    assert(xy_gui_label_set_text(&label, "Hello") == 0);
    assert(xy_gui_label_get_text_width(&label) > 0);
    assert(xy_gui_label_get_text_height(&label) > 0);
    assert(xy_gui_label_set_text_align(&label, XY_GUI_ALIGN_CENTER) == 0);
    assert(xy_gui_label_set_word_wrap(&label, true) == 0);
    assert(xy_gui_label_set_ellipsis(&label, true) == 0);
    assert(xy_gui_label_draw(&label, fb, 160, 80) == 0);

    assert(xy_gui_progress_create(&progress, 0, 0, 100, 10, 0, 100) == 0);
    assert(xy_gui_progress_set_value(&progress, 150) == 0);
    assert(xy_gui_progress_get_value(&progress) == 100);
    assert(xy_gui_progress_set_value(&progress, -10) == 0);
    assert(xy_gui_progress_get_value(&progress) == 0);
    assert(xy_gui_progress_set_type(&progress, XY_GUI_PROGRESS_INDETERMINATE) == 0);
    assert(progress.base.ops->draw(&progress.base, fb, 160, 80) == 0);

    assert(xy_gui_slider_create(&slider, 0, 20, 100, 0, 100, XY_GUI_SLIDER_HORIZONTAL) == 0);
    assert(xy_gui_slider_set_step(&slider, 10) == 0);
    assert(xy_gui_slider_set_value(&slider, 55) == 0);
    assert(xy_gui_slider_get_value(&slider) == 50);
    assert(xy_gui_slider_value_to_pos(&slider, 100) > xy_gui_slider_value_to_pos(&slider, 0));
    assert(xy_gui_slider_pos_to_value(&slider, 999) >= 90);
    reset_cb();
    assert(xy_gui_slider_set_value_changed_cb(&slider, widget_cb, (void*)0x1234) == 0);
    assert(xy_gui_slider_set_continuous(&slider, true) == 0);
    ev = xy_gui_event_create_touch(XY_GUI_EVENT_TOUCH_DOWN, 80, 30);
    assert(xy_gui_slider_update(&slider, &ev) == 0);
    assert(g_cb_count == 1);
    assert(xy_gui_slider_show_ticks(&slider, true) == 0);
    assert(xy_gui_slider_show_value(&slider, true) == 0);
    assert(xy_gui_slider_draw(&slider, fb, 160, 80) == 0);

    assert(xy_gui_container_create(&container, 0, 0, 120, 70) == 0);
    assert(xy_gui_container_set_auto_layout(&container, true) == 0);
    assert(xy_gui_container_set_padding(&container, 3) == 0);
    assert(xy_gui_container_set_spacing(&container, 2) == 0);
    assert(xy_gui_container_add_child(&container, &label.base) == 0);
    assert(container.child_count == 1 && label.base.parent == &container.base);
    assert(xy_gui_container_remove_child(&container, &label.base) == 0);
    assert(container.child_count == 0);

    assert(xy_gui_slider_destroy(&slider) == 0);
    assert(xy_gui_label_destroy(&label) == 0);
}

int main(void)
{
    test_event_queue_and_dispatch();
    test_button_contracts();
    test_checkbox_contracts();
    test_label_progress_slider_container();
    return 0;
}
