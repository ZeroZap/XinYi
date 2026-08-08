# IPC 组件 - 进程间通信

**状态**: ✅ Host-guarded | **Active CTest**: 5 个 | **版本**: 1.1

---

## 📖 简介

XinYi 进程间通信（IPC）组件提供面向本地组件/任务协作的轻量通信机制。当前主线契约已经由 `tests/unit/CMakeLists.txt` 中的 Unity/CTest 目标保护，包含：

- ✅ **Pipe ring buffer** - 调用者提供缓冲区的环形 FIFO。
- ✅ **Broker** - 固定 server/topic/message ID 的本地消息分发。
- ✅ **Message queue** - 固定槽位消息队列，支持优先级、覆盖旧消息与统计。
- ✅ **Observer/Subject** - 进程内观察者回调 fan-out。
- ✅ **Event group** - OSAL event flags 的薄封装；只负责 bit 同步便利 API，不替代 broker/topic。

> 边界：IPC event group 不声明 ISR 安全或跨线程阻塞语义；这些必须先由 OSAL backend 或真实硬件/线程验证证明。

---

## 🚀 快速开始

### Pipe 示例

```c
#include <string.h>
#include "xy_pipe.h"

void pipe_example(void)
{
    uint8_t storage[64];
    uint8_t out[16];
    xy_pipe_t pipe;

    if (xy_pipe_init(&pipe, "uart_rx", storage, sizeof(storage)) != XY_PIPE_OK) {
        return;
    }

    (void)xy_pipe_write(&pipe, (const uint8_t *)"OK", 2U);
    int n = xy_pipe_read(&pipe, out, sizeof(out));
    (void)n;

    (void)xy_pipe_deinit(&pipe);
}
```

### Broker pub/sub 示例

```c
#include "xy_broker.h"

static int sensor_handler(const xy_broker_msg_t *msg, void *user_data)
{
    (void)msg;
    (void)user_data;
    return XY_BROKER_OK;
}

void broker_example(void)
{
    xy_broker_init();
    xy_broker_register_server(XY_BROKER_SERVER_SENSOR, "sensor", sensor_handler, NULL);
    xy_broker_subscribe(XY_BROKER_SERVER_SENSOR, XY_BROKER_TOPIC_SENSOR_DATA,
                        sensor_handler, NULL);

    xy_broker_msg_t msg = {
        .msg_id = XY_BROKER_MSG_SENSOR_DATA,
        .src_server = XY_BROKER_SERVER_SENSOR,
        .topic_id = XY_BROKER_TOPIC_SENSOR_DATA,
        .payload_len = 0U,
    };
    (void)xy_broker_publish(&msg);

    xy_broker_deinit();
}
```

### Message queue 示例

```c
#include "xy_mq.h"

void mq_example(void)
{
    xy_mq_t mq;
    uint8_t payload[] = {1U, 2U, 3U};
    xy_mq_config_t cfg = {
        .msg_size = sizeof(payload),
        .max_msgs = 4U,
        .priority_enabled = true,
        .overwrite_old = false,
    };

    if (xy_mq_init(&mq, &cfg) != XY_MQ_OK) {
        return;
    }

    xy_mq_msg_t tx = {
        .id = 1U,
        .priority = XY_MQ_PRIORITY_NORMAL,
        .data = payload,
        .len = sizeof(payload),
    };
    (void)xy_mq_send(&mq, &tx, 0U);

    uint8_t rx_payload[sizeof(payload)] = {0};
    xy_mq_msg_t rx = {.data = rx_payload, .len = sizeof(rx_payload)};
    (void)xy_mq_recv(&mq, &rx, 0U);

    (void)xy_mq_deinit(&mq);
}
```

### Observer/Subject 示例

```c
#include "xy_observer.h"

static void notify_cb(xy_subject_t *subject, const void *data, void *user_data)
{
    (void)subject;
    (void)data;
    (void)user_data;
}

void observer_example(void)
{
    xy_subject_t subject;
    xy_observer_t observer;

    xy_subject_init(&subject, "state");
    xy_observer_init(&observer, "watcher", notify_cb, NULL);

    xy_subject_attach(&subject, &observer);
    xy_subject_notify(&subject, "changed");

    xy_subject_deinit(&subject);
}
```

### Event group 示例

```c
#include "xy_event_group.h"

void event_group_example(void)
{
    xy_ipc_event_group_t group;
    xy_ipc_event_bits_t matched = 0U;

    if (xy_ipc_event_group_init(&group, "ready") != XY_IPC_EVENT_OK) {
        return;
    }

    (void)xy_ipc_event_group_set(&group, 0x01U, NULL);
    (void)xy_ipc_event_group_wait(&group, 0x01U, XY_IPC_EVENT_WAIT_ANY, 0U, &matched);

    (void)xy_ipc_event_group_deinit(&group);
}
```

---

## 📋 API 参考

### Pipe (`components/ipc/pipe/xy_pipe.h`)

| 函数 | 说明 |
|------|------|
| `xy_pipe_init()` / `xy_pipe_deinit()` | 初始化/释放调用者提供缓冲区的 pipe |
| `xy_pipe_write()` / `xy_pipe_read()` / `xy_pipe_peek()` | 写入、读取、窥视 FIFO 数据 |
| `xy_pipe_available()` / `xy_pipe_is_empty()` / `xy_pipe_is_full()` | 查询 pipe 状态 |
| `xy_pipe_clear()` | 清空 pipe 内容 |

### Broker (`components/ipc/xy_broker/xy_broker.h`)

| API 类别 | 说明 |
|---------|------|
| `xy_broker_init()` / `xy_broker_deinit()` | 初始化/释放全局 broker |
| server registration | 固定 server ID 注册、注销、查找 |
| direct message | 按 server ID 投递固定消息结构 |
| pub/sub | topic ID 订阅、发布、取消订阅 |
| request/response | 请求、响应、超时与统计辅助 |

### Message queue (`components/ipc/inc/xy_mq.h`)

| 函数 | 说明 |
|------|------|
| `xy_mq_init()` / `xy_mq_deinit()` | 初始化/释放固定槽位队列 |
| `xy_mq_send()` / `xy_mq_recv()` | 发送/接收消息，可带 timeout 参数 |
| `xy_mq_try_send()` / `xy_mq_try_recv()` | 非阻塞发送/接收 |
| `xy_mq_get_count()` / `xy_mq_get_free()` | 查询已用/剩余槽位 |
| `xy_mq_clear()` / `xy_mq_get_stats()` | 清空队列和读取统计 |

### Observer (`components/ipc/observer/xy_observer.h`)

| 函数 | 说明 |
|------|------|
| `xy_observer_init()` / `xy_observer_deinit()` | 初始化/释放 observer |
| `xy_subject_init()` / `xy_subject_deinit()` | 初始化/释放 subject |
| `xy_subject_attach()` / `xy_subject_detach()` | 维护 observer 列表 |
| `xy_subject_notify()` | 通知当前 subject 的 observer |
| `xy_subject_observer_count()` / `xy_subject_clear()` | 查询和清空 observer |

### Event group (`components/ipc/inc/xy_event_group.h`)

| 函数 | 说明 |
|------|------|
| `xy_ipc_event_group_init()` / `xy_ipc_event_group_deinit()` | 创建/删除 OSAL event-flags wrapper |
| `xy_ipc_event_group_set()` / `xy_ipc_event_group_clear()` | 设置/清除 bit mask，可返回操作后/前 bit 状态 |
| `xy_ipc_event_group_get()` | 获取当前 bit 状态 |
| `xy_ipc_event_group_wait()` | 按 `XY_IPC_EVENT_WAIT_ANY` / `XY_IPC_EVENT_WAIT_ALL` 等待已置位 bit |

---

## 🧪 验证命令

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target \
  test_ipc_pipe test_ipc_broker test_ipc_mq test_ipc_observer test_ipc_event_group \
  -j$(nproc)
cd build/tests/unit && ctest --output-on-failure \
  -R '^(ipc_pipe|ipc_broker|ipc_mq|ipc_observer|ipc_event_group)$'

# Full unit gate
make test-unit
git diff --check
```

当前 focused IPC CTest 覆盖：

| CTest | 主要契约 |
|-------|----------|
| `ipc_pipe` | init/deinit、读写/peek、empty/full、截断、wraparound |
| `ipc_broker` | lifecycle、server、direct queue、pub/sub、request/response、timeout、debug name |
| `ipc_mq` | FIFO、priority、urgent/drop、overwrite-old、timeout/delay、stats、短接收 buffer metadata |
| `ipc_observer` | init/name guard、attach idempotency、notify dispatch、detach/not-found、capacity、clear/deinit |
| `ipc_event_group` | set/get/clear、wait-any/all、clear-on-success、no-clear、timeout output preservation、post-deinit guard |

---

## Backlog / 边界

1. IPC 仍保持 always-discoverable core component；root `COMPONENT_IPC` 是否需要引入已由 `docs/design/xinyi-ipc-component-config-proposal-2026-08-08.md` 记录为后续独立配置 slice。
2. Event group 只作为 OSAL event flags 薄封装；ISR/threaded wait 语义必须等待 OSAL backend 或真实硬件/线程证据。
3. 后续 IPC 修改应保持路径限定，并至少运行 focused IPC CTest、`make test-unit`、`git diff --check`。

---

*最后更新：2026-08-08 | 维护者：XinYi Team*
