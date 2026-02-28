# GoF 设计模式 (23 种)

**版本**: 1.0  
**最后更新**: 2026-02-28  
**来源**: Gang of Four (GoF) - Design Patterns

---

## 📋 概述

设计模式 (Design Patterns) 是由 Erich Gamma、Richard Helm、Ralph Johnson 和 John Vlissides 四位作者 (合称 Gang of Four，简称 GoF) 在 1994 年提出的 23 种面向对象设计的经典模式。

---

## 🗂️ 模式分类

设计模式根据其**意图**分为三类：**创建型**、**结构型**、**行为型**。

| 分类 | 数量 | 说明 |
|------|------|------|
| **创建型模式** | 5 | 处理对象创建机制 |
| **结构型模式** | 7 | 处理类和对象的组合 |
| **行为型模式** | 11 | 处理对象间的通信 |

---

## 🏗️ 创建型模式 (Creational Patterns)

### 1. Singleton (单例)

| 项目 | 内容 |
|------|------|
| **意图** | 确保类只有一个实例，提供全局访问点 |
| **适用场景** | 配置管理、日志系统、资源池 |

**C 语言实现**:
```c
typedef struct {
    int value;
} Singleton;

static Singleton* instance = NULL;

Singleton* singleton_get_instance(void)
{
    if (!instance) {
        instance = malloc(sizeof(Singleton));
        instance->value = 0;
    }
    return instance;
}
```

---

### 2. Factory Method (工厂方法)

| 项目 | 内容 |
|------|------|
| **意图** | 定义一个创建对象的接口，让子类决定实例化哪个类 |
| **适用场景** | 无法预知对象确切类型、需要解耦对象创建与使用 |

**C 语言实现**:
```c
typedef struct {
    void (*draw)(void* self);
} Shape;

typedef struct {
    Shape base;
} Circle;

typedef struct {
    Shape base;
} Square;

static void circle_draw(void* self) { xy_log_d("Draw Circle\n"); }
static void square_draw(void* self) { xy_log_d("Draw Square\n"); }

Shape* create_circle(void)
{
    Circle* c = malloc(sizeof(Circle));
    c->base.draw = circle_draw;
    return &c->base;
}

Shape* create_square(void)
{
    Square* s = malloc(sizeof(Square));
    s->base.draw = square_draw;
    return &s->base;
}
```

---

### 3. Abstract Factory (抽象工厂)

| 项目 | 内容 |
|------|------|
| **意图** | 提供创建相关或依赖对象家族的接口，无需指定具体类 |
| **适用场景** | 需要创建产品族、确保产品兼容性 |

**C 语言实现**:
```c
typedef struct {
    void (*create_button)(void);
    void (*create_checkbox)(void);
} GUIFactory;

typedef struct {
    GUIFactory base;
} WinFactory;

typedef struct {
    GUIFactory base;
} MacFactory;
```

---

### 4. Builder (建造者)

| 项目 | 内容 |
|------|------|
| **意图** | 分步骤构建复杂对象，分离构造与表示 |
| **适用场景** | 对象内部结构复杂、需要不同表示 |

**C 语言实现**:
```c
typedef struct {
    char* part_a;
    char* part_b;
    char* part_c;
} Product;

typedef struct {
    void (*build_a)(void*);
    void (*build_b)(void*);
    void (*build_c)(void*);
    Product* (*get_product)(void*);
} Builder;
```

---

### 5. Prototype (原型)

| 项目 | 内容 |
|------|------|
| **意图** | 通过复制现有对象创建新对象 |
| **适用场景** | 创建成本高、需要深拷贝/浅拷贝控制 |

**C 语言实现**:
```c
typedef struct {
    void (*clone)(void* self, void* dest);
    int data;
} Prototype;

void prototype_clone(void* self, void* dest)
{
    memcpy(dest, self, sizeof(Prototype));
}
```

---

## 🏛️ 结构型模式 (Structural Patterns)

### 6. Adapter (适配器)

| 项目 | 内容 |
|------|------|
| **意图** | 转换接口，使不兼容的类能一起工作 |
| **适用场景** | 复用现有类但接口不匹配 |

**C 语言实现**:
```c
/* 目标接口 */
typedef struct {
    void (*request)(void);
} Target;

/* 适配者 */
typedef struct {
    void (*specific_request)(void);
} Adaptee;

/* 适配器 */
typedef struct {
    Target base;
    Adaptee* adaptee;
} Adapter;
```

---

### 7. Bridge (桥接)

| 项目 | 内容 |
|------|------|
| **意图** | 分离抽象与实现，使两者可独立变化 |
| **适用场景** | 避免类爆炸、运行时切换实现 |

**C 语言实现**:
```c
typedef struct {
    void (*operation_impl)(void);
} Implementor;

typedef struct {
    Implementor* impl;
    void (*operation)(void* self);
} Abstraction;
```

---

### 8. Composite (组合)

| 项目 | 内容 |
|------|------|
| **意图** | 将对象组合成树形结构表示"部分 - 整体"层次 |
| **适用场景** | 树形结构、统一处理单个/组合对象 |

**C 语言实现**:
```c
typedef struct Component {
    struct Component** children;
    int child_count;
    void (*add)(struct Component*, struct Component*);
    void (*draw)(struct Component*);
} Component;
```

---

### 9. Decorator (装饰器)

| 项目 | 内容 |
|------|------|
| **意图** | 动态添加职责到对象，比继承更灵活 |
| **适用场景** | 需要动态添加功能、避免类爆炸 |

**C 语言实现**:
```c
typedef struct {
    void (*draw)(void*);
} Component;

typedef struct {
    Component* wrapped;
    void (*draw)(void* self);
} Decorator;
```

---

### 10. Facade (外观)

| 项目 | 内容 |
|------|------|
| **意图** | 为子系统提供统一高层接口 |
| **适用场景** | 简化复杂子系统使用、降低耦合 |

**C 语言实现**:
```c
typedef struct {
    void (*startup)(void);
    void (*read_data)(void);
    void (*jump)(void);
} CPU;

typedef struct {
    CPU* cpu;
    void (*run)(void* self);
} ComputerFacade;
```

---

### 11. Flyweight (享元)

| 项目 | 内容 |
|------|------|
| **意图** | 共享细粒度对象，减少内存使用 |
| **适用场景** | 大量相似对象、内存敏感场景 |

**C 语言实现**:
```c
typedef struct {
    char* intrinsic_state;
} Flyweight;

typedef struct {
    Flyweight* pool[100];
    int count;
    Flyweight* (*get)(void*, const char*);
} FlyweightFactory;
```

---

### 12. Proxy (代理)

| 项目 | 内容 |
|------|------|
| **意图** | 为对象提供代理以控制访问 |
| **适用场景** | 延迟加载、访问控制、日志记录 |

**C 语言实现**:
```c
typedef struct {
    void (*request)(void);
} Subject;

typedef struct {
    Subject* real;
    void (*request)(void* self);
} Proxy;

void proxy_request(void* self)
{
    Proxy* p = (Proxy*)self;
    xy_log_d("Before request\n");
    p->real->request();
    xy_log_d("After request\n");
}
```

---

## 🔄 行为型模式 (Behavioral Patterns)

### 13. Chain of Responsibility (责任链)

| 项目 | 内容 |
|------|------|
| **意图** | 使多个对象都有机会处理请求，连成一条链 |
| **适用场景** | 多个对象可处理请求、动态指定处理者 |

**C 语言实现**:
```c
typedef struct Handler {
    struct Handler* next;
    void (*handle)(struct Handler*, int request);
} Handler;

void handle_request(Handler* chain, int req)
{
    while (chain) {
        chain->handle(chain, req);
        chain = chain->next;
    }
}
```

---

### 14. Command (命令)

| 项目 | 内容 |
|------|------|
| **意图** | 封装请求为对象，支持参数化、队列、撤销 |
| **适用场景** | 命令队列、撤销/重做、宏命令 |

**C 语言实现**:
```c
typedef struct {
    void (*execute)(void*);
    void (*undo)(void*);
} Command;

typedef struct {
    Command* cmd;
    void (*press)(void* self);
} Button;
```

---

### 15. Iterator (迭代器)

| 项目 | 内容 |
|------|------|
| **意图** | 顺序访问聚合对象元素，无需暴露内部表示 |
| **适用场景** | 统一遍历不同集合、简化集合接口 |

**C 语言实现**:
```c
typedef struct {
    void* (*next)(void*);
    int (*has_next)(void*);
} Iterator;

typedef struct {
    Iterator* (*create_iterator)(void*);
} Aggregate;
```

---

### 16. Mediator (中介者)

| 项目 | 内容 |
|------|------|
| **意图** | 封装对象交互，使对象无需显式引用彼此 |
| **适用场景** | 多对多通信复杂、对象耦合严重 |

**C 语言实现**:
```c
typedef struct {
    void (*notify)(void* sender, const char* event);
} Mediator;

typedef struct {
    Mediator* mediator;
    void (*send)(void* self, const char* msg);
} Colleague;
```

---

### 17. Memento (备忘录)

| 项目 | 内容 |
|------|------|
| **意图** | 捕获对象内部状态并外部保存，用于恢复 |
| **适用场景** | 撤销操作、状态快照、检查点 |

**C 语言实现**:
```c
typedef struct {
    char* state;
} Memento;

typedef struct {
    Memento* (*save)(void*);
    void (*restore)(void*, Memento*);
} Originator;

typedef struct {
    Memento** history;
    int count;
} Caretaker;
```

---

### 18. Observer (观察者)

| 项目 | 内容 |
|------|------|
| **意图** | 定义一对多依赖，对象变化时自动通知依赖者 |
| **适用场景** | 事件系统、MVVM/MVC、发布订阅 |

**C 语言实现**:
```c
typedef struct Observer {
    void (*update)(struct Observer*, const char* data);
} Observer;

typedef struct {
    Observer** observers;
    int count;
    void (*attach)(void*, Observer*);
    void (*notify)(void*, const char*);
} Subject;
```

---

### 19. State (状态)

| 项目 | 内容 |
|------|------|
| **意图** | 允许对象在内部状态改变时改变行为 |
| **适用场景** | 状态驱动行为、消除条件分支 |

**C 语言实现**:
```c
typedef struct State {
    void (*handle)(struct State*);
} State;

typedef struct {
    State* current_state;
    void (*set_state)(void*, State*);
    void (*request)(void*);
} Context;
```

---

### 20. Strategy (策略)

| 项目 | 内容 |
|------|------|
| **意图** | 定义算法族，封装并使其可互换 |
| **适用场景** | 多种算法可替换、避免条件分支 |

**C 语言实现**:
```c
typedef struct {
    int (*execute)(int a, int b);
} Strategy;

typedef struct {
    Strategy* strategy;
    void (*set_strategy)(void*, Strategy*);
    int (*run)(void*, int, int);
} Context;
```

---

### 21. Template Method (模板方法)

| 项目 | 内容 |
|------|------|
| **意图** | 定义算法骨架，将步骤延迟到子类 |
| **适用场景** | 算法框架固定、部分步骤可变 |

**C 语言实现**:
```c
typedef struct {
    void (*step1)(void*);
    void (*step2)(void*);
    void (*template_method)(void* self);
} AbstractClass;
```

---

### 22. Visitor (访问者)

| 项目 | 内容 |
|------|------|
| **意图** | 在不修改元素类前提下定义新操作 |
| **适用场景** | 对象结构稳定、操作频繁变化 |

**C 语言实现**:
```c
typedef struct Visitor Visitor;

typedef struct ElementA {
    void (*accept)(Visitor*);
    int data_a;
} ElementA;

typedef struct ElementB {
    void (*accept)(Visitor*);
    int data_b;
} ElementB;

struct Visitor {
    void (*visit_a)(ElementA*);
    void (*visit_b)(ElementB*);
};
```

---

### 23. Interpreter (解释器)

| 项目 | 内容 |
|------|------|
| **意图** | 定义语言文法表示，使用解释器解释句子 |
| **适用场景** | DSL 设计、脚本引擎 |

**C 语言实现**:
```c
typedef struct {
    void (*interpret)(void* context);
} Expression;

typedef struct {
    Expression* left;
    Expression* right;
} TerminalExpression;
```

---

## 📊 模式分类汇总表

| 分类 | 模式名称 | 核心意图 |
|------|----------|----------|
| **创建型** | Singleton | 确保唯一实例 |
| | Factory Method | 子类决定实例化哪个类 |
| | Abstract Factory | 创建产品族 |
| | Builder | 分步构建复杂对象 |
| | Prototype | 复制现有对象 |
| **结构型** | Adapter | 接口转换 |
| | Bridge | 抽象与实现分离 |
| | Composite | 树形部分 - 整体结构 |
| | Decorator | 动态添加职责 |
| | Facade | 统一高层接口 |
| | Flyweight | 共享细粒度对象 |
| | Proxy | 控制访问 |
| **行为型** | Chain of Responsibility | 链式传递请求 |
| | Command | 封装请求为对象 |
| | Iterator | 顺序访问集合 |
| | Mediator | 封装对象交互 |
| | Memento | 保存/恢复状态 |
| | Observer | 一对多依赖通知 |
| | State | 状态驱动行为 |
| | Strategy | 算法可互换 |
| | Template Method | 定义算法骨架 |
| | Visitor | 新增操作不修改类 |
| | Interpreter | 解释语言文法 |

---

## 🎯 嵌入式开发应用

| 模式 | 嵌入式应用场景 |
|------|---------------|
| **Singleton** | 硬件资源管理器、配置管理器 |
| **Observer** | 传感器事件系统、按键通知 |
| **State** | 状态机实现、协议状态管理 |
| **Strategy** | 算法切换 (如滤波算法) |
| **Command** | 按键命令队列、远程控制 |
| **Factory Method** | 设备驱动创建 |
| **Adapter** | 遗留代码适配、HAL 层封装 |
| **Facade** | 子系统接口 (如通信栈) |

---

## 🔗 相关文档

- [代码风格指南](../design/Code_Style_Design_Guide.md)
- [TAOCP PDF 核心知识提炼](../reference/TAOCP_PDF 核心知识提炼.md)
- [C 语言编程技巧](../rules/misc/c_programming_tips.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
