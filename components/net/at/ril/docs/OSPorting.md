# RTOS移植指南


这一章以demo例程为例，介绍RIL基于Free-RTOS的移植。同时，这种移植方法，同样也适用于其它RTOS平台。

文件ril/port/ril_port.h声明了与RTOS相关的移植接口，基本上所有的RTOS都会提供这些接口，包括下面几项内容：

1. 时间与延时
2. 提供信号量相关操作接口，包含创建、等待、释放等操作
3. 进入/退出临界区
4. 内存分配与释放
5. 日志输出


## 新建文件
参考ril/port/ril_template.c模板新建一个新文件ril_freertos.c，并把它添加到你的工程目录下。

![](/images/porting-file.png)

## 时间与延时

第1部分是时间相关，包含获取系统时间及延时功能。

```c
/**
 * @brief	   获取当前系统毫秒数
 */
unsigned int ril_get_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void ril_delay(unsigned int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
```

## 信号量

!>  由于Free-RTOS版本差异，对于信号量操作可能不太一样，请以实际版本为主。

RIL运行在多任务的系统之上，在处理模组的AT命令通信、Socket数据收发时都需要进行同步以及共享资源保护，以免引起冲突，造成混乱。信号量是由"ril_sem_t”的类型定义在ril_port.h文件中，它实际是一个指向void *类型的指针，这样做的好处显而易见，因为它可以指向任何类型的信号量指针，不会因为移植到一个新的RTOS上需要再重新定义。

```c
/**
 * @brief	   创建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
ril_sem_t ril_sem_new(int value)
{
    return xQueueCreateCountingSemaphore(100, value);
}

/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间(ms为单位)
 * @return     true - 成功获取到信号量, false - 等待超时  
 * @note       如果"timeout"参数非零，线程应该仅在指定的时间内阻塞(以单位为毫秒)。
 *             如果"timeout"参数为零,则任务应该一直阻塞,直到信号量发出信号。
 */
bool ril_sem_wait(ril_sem_t s, unsigned int timeout)
{
    return xQueueSemaphoreTake(s, timeout) == pdTRUE;
}

/**
 * @brief	   发送信号量
 * @params[in] s - 信号量
 * @return     none
 */  
void ril_sem_post(ril_sem_t s)
{
    xQueueGenericSend(s, NULL, 0, queueSEND_TO_BACK );
}
```

## 系统临界保护

RIL内部还有一些比较运行频繁，代码短暂程序的片段，如异步作业及系统通知的链表操作、Socket ID分配等，它们作为公共资源一部分同样需要进行互斥访问保护。使用互斥量是一种常用的方式，但是存在内存消耗及任务切换的开销，所以这些代码使用临界区保护方式作为补充，并提供一个轻量级的互斥访问手段。

```c
/**
 * @brief	   进入临界区
 * @return     none
 */  
void ril_enter_critical(void)
{
    portTICK_TYPE_ENTER_CRITICAL();
}

/*
 * @brief	   退入临界区
 * @return     none
 */  
void ril_exit_critical(void)
{
   portTICK_TYPE_EXIT_CRITICAL();
}
```

## 内存管理

```c
/**
 * @brief	   内存分配
 * @params[in] nbytes - 分配字节数
 */  
void *ril_malloc(int nbytes)
{
    return pvPortMalloc(nbytes);
}

/**
 * @brief	   内存释放
 * @params[in] p - 待释放指针
 */  
void ril_free(void *p)
{
    vPortFree(p);
}
```

## 日志管理

RIL根据运行时日志类型及日志划分了4个等级：

| 日志等级     | 描述                                 |
| ------------ | ------------------------------------ |
| RIL_LOG_DBG  | 调试信息，显示所有AT指令通信交互细节 |
| RIL_LOG_INFO | 重要信息，显示RIL运行状态            |
| RIL_LOG_WARN | 警告信息，表明会出现潜在错误的情形   |
| RIL_LOG_ERR  | 错误信息，严重影响RIL正常运行的情形  |

这里为了方便演示，只输出高于INFO等级的日志。

```c
/**
 * @brief      调试输出
 * @params[in] level - log等级,参考RIL_LOG_XXX
 * @params[in] fmt   - 格式化输出
 */ 
void ril_log(int level, const char *fmt, ...)
{
    va_list   args;
    
    if (level < RIL_LOG_INFO)          //只输出高于INFO等级的日志，如果你希望看到所有AT指令交互信息，可以屏蔽此行。
        return;
    
    ril_enter_critical();
    
    printf("[RIL]:");
    va_start(args, fmt); 
    vprintf(fmt, args);
    va_end(args);
    
    ril_exit_critical();
}
```

