# IPC 组件 - 进程间通信

**状态**: ✅ 完善 | **测试**: 14 用例 | **版本**: 1.0

---

## 📖 简介

XinYi 进程间通信（IPC）组件提供管道、观察者模式等通信机制。

### 核心特性

- ✅ **管道通信** - 环形缓冲区实现
- ✅ **观察者模式** - 发布/订阅机制
- ✅ **线程安全** - 支持多任务访问

---

## 🚀 快速开始

### 管道示例

```c
#include "xy_pipe.h"

int main(void) {
    uint8_t buffer[256];
    xy_pipe_t pipe;
    
    xy_pipe_init(&pipe, "test", buffer, sizeof(buffer));
    
    // 写入数据
    const char *msg = "Hello!";
    xy_pipe_write(&pipe, (const uint8_t *)msg, strlen(msg));
    
    // 读取数据
    char read_buf[64];
    int len = xy_pipe_read(&pipe, (uint8_t *)read_buf, sizeof(read_buf));
    
    xy_pipe_deinit(&pipe);
    return 0;
}
```

### 观察者模式示例

```c
#include "xy_observer.h"

static void my_callback(xy_subject_t *subject, const void *data, void *user_data) {
    printf("Received: %s\n", (const char *)data);
}

int main(void) {
    xy_subject_t subject;
    xy_observer_t observer;
    
    xy_subject_init(&subject, "MySubject");
    xy_observer_init(&observer, "MyObserver", my_callback, NULL);
    
    xy_subject_attach(&subject, &observer);
    xy_subject_notify(&subject, "Hello!");
    
    xy_subject_deinit(&subject);
    return 0;
}
```

---

## 📋 API 参考

### Pipe

| 函数 | 说明 |
|------|------|
| `xy_pipe_init()` | 初始化管道 |
| `xy_pipe_write()` | 写入数据 |
| `xy_pipe_read()` | 读取数据 |
| `xy_pipe_peek()` | 窥视数据 |
| `xy_pipe_clear()` | 清空管道 |

### Observer

| 函数 | 说明 |
|------|------|
| `xy_observer_init()` | 初始化观察者 |
| `xy_subject_init()` | 初始化主题 |
| `xy_subject_attach()` | 附加观察者 |
| `xy_subject_detach()` | 分离观察者 |
| `xy_subject_notify()` | 通知观察者 |

---

## 🧪 测试用例

IPC 组件包含 14 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| Pipe | 9 |
| Observer | 5 |

运行测试：
```bash
ctest -R test_ipc --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
