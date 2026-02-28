# GUI 组件 - 图形用户界面

**状态**: 📋 基础 | **测试**: 0 用例 | **版本**: 1.0

---

## 📖 简介

XinYi GUI 组件提供轻量级的嵌入式图形用户界面框架。

### 核心特性

- 📋 **对象管理** - 标签/按钮/条形图等
- 📋 **绘图函数** - 像素/直线/矩形/字符
- 📋 **显示器驱动** - 可配置的显示驱动接口
- 📋 **RGB565 颜色** - 16 位颜色支持

---

## 🚀 快速开始

```c
#include "xy_gui.h"

int main(void) {
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = {
        .init = display_init,
        .draw_pixel = display_draw_pixel,
        .draw_line = display_draw_line,
        .flush = display_flush,
    };
    
    // 初始化 GUI
    xy_gui_init(&gui, 320, 240, &drv);
    
    // 清屏
    xy_gui_clear(&gui, GUI_COLOR_WHITE);
    
    // 绘制对象
    xy_gui_fill_rect(&gui, 10, 10, 100, 50, GUI_COLOR_BLUE);
    xy_gui_draw_string(&gui, 20, 20, "Hello!", GUI_COLOR_WHITE);
    
    // 刷新
    xy_gui_flush(&gui);
    
    return 0;
}
```

---

## 📋 API 参考

### 绘图

| 函数 | 说明 |
|------|------|
| `xy_gui_draw_pixel()` | 绘制像素 |
| `xy_gui_draw_line()` | 绘制直线 |
| `xy_gui_draw_rect()` | 绘制矩形 |
| `xy_gui_fill_rect()` | 填充矩形 |
| `xy_gui_draw_char()` | 绘制字符 |
| `xy_gui_draw_string()` | 绘制字符串 |

### 对象管理

| 函数 | 说明 |
|------|------|
| `xy_gui_obj_create()` | 创建对象 |
| `xy_gui_obj_delete()` | 删除对象 |
| `xy_gui_obj_set_text()` | 设置文本 |
| `xy_gui_obj_set_color()` | 设置颜色 |
| `xy_gui_obj_redraw()` | 重绘对象 |

---

## 📝 待完成任务

- [ ] 实现 xy_gui.c
- [ ] 添加单元测试
- [ ] 添加字体支持
- [ ] 添加更多 UI 组件

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
