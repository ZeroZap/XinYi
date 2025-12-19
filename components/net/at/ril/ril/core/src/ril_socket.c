/******************************************************************************
 * @brief    ril socket
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version. 
 * 2021-04-23     Morro        Fix the issue of not exiting the critical region 
 *                             correctly when allocating Socket id.
 * 2021-05-04     Morro        Fix the issue of repeatedly closing the socket 
 *                             when the remote host is disconnected.
 * 2021-12-08     Morro        Fix the problem of receiving abnormal data under 
 *                             multitasking system
 ******************************************************************************/
#include "ril_core.h"
#include "ril_socket.h"
#include "linux_list.h"
#include "ringbuffer.h"
#include "ril_socket_internal.h"
#include "ril_device_impl.h"
#include <string.h>

#define RIL_DEV  (&get_ril_obj()->dev)

#define SOCK_FD(s) (int)(s)

/*socket ---------------------------------------------------------------------*/
typedef struct {      
    socket_base_t    base;                             /* socket 基本信息*/
    struct list_head node;                             /* 链表节点*/    
    socket_event_t   event;                            /* socket事件 */
    ring_buf_t       rb;                               /* 环形缓冲区管理*/
    unsigned int     conn_failed_wait;                 /* 连接失败等待时间*/
    unsigned int     conn_timer, send_timer, tick;     /* 定时器*/    

    unsigned short   unread_data_size;                 /* 剩余待读取的数据大小*/    
    unsigned char    connstat,sendstat;                /* 连接状态,发送状态*/
    unsigned char    conn_failed_cnt;                  /* 连接失败计数*/    
    unsigned char    recv_incomming : 1;               /* 数据接收到来标志*/
    unsigned char    recv_event     : 1;               /* 接收事件处理标志*/
    unsigned char    recvbuf[0];                       /* 接收缓冲区*/
} socket_obj_t;

/* Private variables ---------------------------------------------------------*/
static LIST_HEAD(sock_list); 

static unsigned int sock_id_tbl;                   //socket id表

const char *socket_status_desc[SOCK_STAT_MAX] = {
    "Unknow", "Busy", "Completed", "Failed", "Timeout"
};
/**
 * @brief      分配SOCKET ID
 */
static bool socket_id_alloc(int *id)
{
    int i;
    ril_enter_critical();
    for (i = 0; i < 32; i++) {
        if ( (sock_id_tbl & (1 << i)) == 0) {
            *id = i;
            sock_id_tbl |= 1 << i;
            ril_exit_critical();
            return true;
        }        
    }
    ril_exit_critical(); 
    RIL_WARN("Socket id Allocation failure:%08X\r\n", sock_id_tbl);        
    return false;
}
/**
 * @brief      释放SOCKET ID
 */
static void socket_id_free(int id)
{
    ril_enter_critical();
    sock_id_tbl &= ~(1 << id);
    ril_exit_critical();
}


/**
 * @brief      事件上报
 */
static void sock_event_invoke(socket_obj_t *s, socket_event_type type)
{
    if (s->event)
        s->event(SOCK_FD(s), type);
}
/**
 * @brief       更新socket连接状态
 */
static void update_sock_connstat(socket_obj_t *s, sock_request_status status)
{
    RIL_INFO("Socket %d connect %s.\r\n", s->base.id,
             socket_status_desc[status % SOCK_STAT_MAX]);
    
    if (status != SOCK_STAT_DONE && status != SOCK_STAT_BUSY)
        ril_sock_disconnect(SOCK_FD(s));
    
    s->connstat = status;
    
    sock_event_invoke(s, SOCK_EVENT_CONN);                    /* 上报连接事件 */
        
}
/**
 * @brief       更新socket发送状态
 */
static void update_sock_sendstat(socket_obj_t *s, sock_request_status status)
{
    RIL_INFO("Socket %d send %s.\r\n", s->base.id,
             socket_status_desc[status % SOCK_STAT_MAX]);
    
    s->sendstat = status;
    
    sock_event_invoke(s, SOCK_EVENT_SEND);         /* 上报发送事件*/
}
/**
 * @brief       注册socket
 */
static  void ril_sock_register(socket_obj_t *s)
{
    ril_enter_critical();
    list_add_tail(&s->node, &sock_list);
    ril_exit_critical();
}

/**
 * @brief       移除socket
 */
static void ril_sock_unregister(socket_obj_t *s)
{
    ril_enter_critical();
    list_del(&s->node);
    ril_exit_critical();
}


 /**
   * Returns the smallest power of two >= its argument, with several caveats:
   * If the argument is negative but not Integer.MIN_VALUE, the method returns
   * zero. If the argument is > 2^30 or equal to Integer.MIN_VALUE, the method
   * returns Integer.MIN_VALUE. If the argument is zero, the method returns
   * zero.
   *
   */
int roundUpToPowerOfTwo(int i) 
{
    i--; // If input is a power of two, shift its high-order bit right.
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    return i + 1;
}

/**
 * @brief       创建socket
 * @param[in]   s       - socket
 * @param[in]   e       - socket事件回调接口(如果不需要,可以填NULL)  
 * @param[in]   bufsize - 接收缓冲区大小(自动对齐2的幂次)，如果填0表示自动分配
 * @return      RIL_INVALID_SOCKET -  无效socket, 其它值 - socket 描述符
 */
ril_socket_t ril_sock_create(socket_event_t e, unsigned int bufsize)
{  
    int id;
    socket_obj_t *s;
    //计算接收缓冲区大小
    bufsize = bufsize == 0 ? DEF_SOCK_RECV_BUFSIZE : bufsize;
    bufsize = roundUpToPowerOfTwo(bufsize);
    s = (socket_obj_t *)ril_malloc(sizeof(socket_obj_t) + bufsize);
    if (s == NULL) {
        RIL_ERR("Socket creation failed,out of memory.\r\n");        
        return RIL_INVALID_SOCKET;
    }  
    memset(s, 0, sizeof(socket_obj_t));
    s->event = e;    
    if (!ring_buf_init(&s->rb, s->recvbuf, bufsize)) /* 初始化接收缓冲区*/
        goto Error;   
    if (!socket_id_alloc(&id)) {
        goto Error;
    } else {
        s->base.id = id;
        ril_sock_register(s);
        return (int)s;
    }
Error:
    ril_free(s);
    return RIL_INVALID_SOCKET;    
}

/**
 * @brief  通过id查询socket
 */
socket_base_t *find_socket_by_id(int id)
{
    socket_obj_t *s;
    struct list_head *list ,*n = NULL;     
    list_for_each_safe(list, n, &sock_list) {
        s = list_entry(list, socket_obj_t, node);    
        if (s->base.id == id)
            return &s->base;
    }   
    RIL_WARN("Unknow socket id:%d.\r\n", id);
    return NULL;
}

/**
 * @brief       为socket设置附属数据
 * @param[in]   s       - socket
 * @param[in]   tag     - 附属数据
 */
void set_socket_tag(socket_base_t *s, void *tag)
{
    s->tag = tag;
}

/**
 * @brief  通过socket 附属数据查询socket
 */
socket_base_t *find_socket_by_tag(void *tag)
{
    socket_obj_t *s;
    struct list_head *list ,*n = NULL;     
    list_for_each_safe(list, n, &sock_list) {
        s = list_entry(list, socket_obj_t, node);    
        if (s->base.tag == tag)
            return &s->base;
    }   
    return NULL;
}

/**
 * @brief  当出现连接异常时,插入连接等待时间,避免频繁尝试
 */
static bool socket_conn_wait(socket_obj_t *s)
{
    unsigned int wait_time;
    if (s->conn_failed_cnt) {
        wait_time = s->conn_failed_cnt % 10 * 6000;
        if (!ril_istimeout(s->conn_failed_wait, wait_time))
            return false;
    }
    return true;
}

/**
 * @brief       socket 异步连接操作
 * @param[in]   sockfd   - socket描述符
 * @param[in]   host     - 远程主机
 * @param[in]   port     - 远程端口
 * @param[in]   type     - socket 类型( 0 -TCP, 1 - UDP)
 * @return      RIL_OK   - 执行成功, 其它值 - 异常
 */
int ril_sock_connect_async(ril_socket_t sockfd, const char *host, unsigned short port,
                            ril_socket_type type)
{    
    socket_obj_t *s = (socket_obj_t *)sockfd;
    if (s->connstat == SOCK_STAT_BUSY || !ril_isonline() || !socket_conn_wait(s))
        return RIL_REJECT;
    
    s->base.host       = host;
    s->base.port       = port;
    s->base.type       = type;
    
    s->conn_timer = ril_get_ms();
    s->sendstat   = SOCK_STAT_UNKNOW;
    if(ril_dev_sock_connect(RIL_DEV, &s->base) != RIL_OK) {
        s->connstat = SOCK_STAT_FAILED;  
        s->conn_failed_cnt++;                                /*统计失败次数*/
        s->conn_failed_wait = ril_get_ms();
        return RIL_ERROR;
    } else {
        s->conn_failed_cnt = 0;
        s->connstat = SOCK_STAT_BUSY;  
        return RIL_OK;
    }
}

/**
 * @brief       socket 数据发送操作(阻塞直到连接结束)
 * @param[in]   sockfd   - socket描述符
 * @param[in]   host     - 远程主机
 * @param[in]   port     - 远程端口
 * @param[in]   type     - 连接类型(RIL_SOCK_XXX)
 * @return      RIL_OK  - 连接成功, 其它值 - 异常
 */
int ril_sock_connect(ril_socket_t sockfd, const char *host, unsigned short port, 
                     ril_socket_type type)
{    
    int ret;
    socket_obj_t *s = (socket_obj_t *)sockfd;
    ret = ril_sock_connect_async(SOCK_FD(s), host, port, type);
    if (ret != RIL_OK)
        return ret;
    
    while (s->connstat == SOCK_STAT_BUSY) {
        ril_delay(1);//yeild
    }
    return s->connstat == SOCK_STAT_DONE ? RIL_OK : RIL_ERROR;
}

/**
 * @brief       socket 数据发送操作(非阻塞)
 * @param[in]   s   - socket
 * @param[in]   buf - 数据缓冲区
 * @param[in]   len - 缓冲区长度
 * @return      RIL_OK   - 执行成功, 其它值 - 异常
 */
int ril_sock_send_async(ril_socket_t sockfd, const void *buf, unsigned int len)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    bool ret;
    ril_obj_t *r = get_ril_obj();
    if (!ril_sock_online(sockfd) || s->sendstat == SOCK_STAT_BUSY)
        return RIL_REJECT;
    s->send_timer = ril_get_ms();
    ret = ril_dev_sock_send(RIL_DEV, &s->base, buf, len);
    if(ret != RIL_OK)   //发送失败之后,更新连接状态
        s->connstat = ril_dev_sock_conn_status(&r->dev, &s->base); 
    else 
        s->sendstat = SOCK_STAT_BUSY;
    return ret;
}

/**
 * @brief       socket 数据发送操作(阻塞直到发送结束)
 * @param[in]   s   - socket
 * @param[in]   buf - 数据缓冲区
 * @param[in]   len - 缓冲区长度
 * @return      RIL_OK   - 发送成功, 其它值 - 异常
 */
int ril_sock_send(ril_socket_t sockfd, const void *buf, unsigned int len)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    unsigned int send_timer;
    int retry = 0;
    sock_request_status status;
    ril_obj_t *r = get_ril_obj();
    
    if (ril_sock_send_async(sockfd, buf, len) != RIL_OK)
        return RIL_ERROR;
    send_timer = ril_get_ms();
    //Wait for sending to complete.
    while (!ril_istimeout(send_timer, MAX_SOCK_SEND_TIME * 1000) && 
           ril_sock_online(sockfd)) {
        status = ril_dev_sock_send_status(&r->dev, &s->base);
        if (status != SOCK_STAT_BUSY) {
            update_sock_sendstat(s, status);
            break;
        }
        ril_delay(20 * retry * retry);//yeild
        if (retry < 10)
            retry++;
    }
    
    if (!ril_sock_online(sockfd))
        update_sock_sendstat(s, SOCK_STAT_FAILED);
    else if (ril_istimeout(send_timer, MAX_SOCK_SEND_TIME * 1000))
        update_sock_sendstat(s, SOCK_STAT_TIMEOUT);
    
    return ril_sock_sendstat(sockfd) == SOCK_STAT_DONE ? RIL_OK : RIL_ERROR;
}

/**
 * @brief       接收数据(非阻塞)
 * @param[in]   s   - socket
 * @param[in]   buf - 接收缓冲区
 * @param[in]   len - 缓冲区长度
 * @retval      实际接收到数据长度
 */
unsigned int ril_sock_recv(ril_socket_t sockfd, void *buf, unsigned int len)
{
    unsigned int ret;
    socket_obj_t *s = (socket_obj_t *)sockfd;
    if (ring_buf_len(&s->rb) == 0)
        return 0;
    ril_enter_critical();    
    ret = ring_buf_get(&s->rb, buf, len);
    ril_exit_critical();
    return ret;
    

}

/**
 * @brief       断开连接
 */
int ril_sock_disconnect(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    s->connstat = SOCK_STAT_UNKNOW;
    s->sendstat = SOCK_STAT_UNKNOW;
    return ril_dev_sock_disconnect(RIL_DEV, &s->base);
}

/**
 * @brief       指示socket已经连接服务器
 */
bool ril_sock_online(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    return s->connstat == SOCK_STAT_DONE;
}

/**
 * @brief       指示socket正在进行连接/发送状态
 */
bool ril_sock_busy(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    return s->connstat == SOCK_STAT_BUSY || s->sendstat == SOCK_STAT_BUSY;
}

/**
 * @brief 销毁socket
 */
void ril_sock_destroy(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    if (ril_sock_online(sockfd))
        ril_sock_disconnect(sockfd);
    
    socket_id_free(s->base.id);
    
    ril_sock_unregister(s);
    ril_free(s);
}

/**
 * @brief       连接状态
 */
sock_request_status ril_sock_connstat(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    return (sock_request_status)s->connstat;
}

/**
 * @brief       发送状态
 */
sock_request_status ril_sock_sendstat(ril_socket_t sockfd)
{
    socket_obj_t *s = (socket_obj_t *)sockfd;
    return (sock_request_status)s->sendstat;
}

//通知处理
static void sock_notify_process(async_work_t *w, void *object, void *params)
{
    socket_obj_t *s = (socket_obj_t *)object;
    sock_notify_type type = (sock_notify_type)((int)(params));

    switch (type) {
    case SOCK_NOTFI_ONLINE:                            //上线事件
        update_sock_connstat(s, SOCK_STAT_DONE);
        break;
    case SOCK_NOTFI_OFFLINE:                           //掉线事件
        RIL_WARN("The remote host is disconnected.\r\n");
        //ril_sock_disconnect(SOCK_FD(s));
        update_sock_connstat(s, SOCK_STAT_FAILED);
        if (s->sendstat == SOCK_STAT_BUSY) {          //当前正在发送数据
            update_sock_sendstat(s, SOCK_STAT_FAILED);
        }
        break;   
    case SOCK_NOTFI_DATA_REPORT:                       //收到数据
        sock_event_invoke(s, SOCK_EVENT_RECV);
        s->recv_event = false;
        break;
    case SOCK_NOTFI_SEND_FAILED:                       //发送失败 
        update_sock_sendstat(s, SOCK_STAT_FAILED);
        break;
    case SOCK_NOTFI_SEND_SUCCESS:                      //发送成功
        update_sock_sendstat(s, SOCK_STAT_DONE);
        break; 
    default:
        break;
    }
}

/**
 * @brief  socket数据接收(用于处理主动上报)
 */
static void ril_sock_data_input(socket_obj_t *s,  const void *buf, int size)
{         
    for (int i = 0; i < 5; i++) {
        ril_enter_critical();
        size -= ring_buf_put(&s->rb, (unsigned char *)buf, size);
        ril_exit_critical();
        if (size == 0)
            return;
        ril_delay(10);                                //等待上层读取
    }
    RIL_ERR("Socket %d  buffer full.\r\n", s->base.id);
}

/*******************************************************************************
 * @brief       产生socket通知
 * @param[in]   s      - socket
 * @param[in]   type   - 通知类型
 * @param[in]   data   - 通用数据,参考sock_notify_type描述
 * @return      none
 ******************************************************************************/
void ril_socket_notify(socket_base_t *base, sock_notify_type type, void *data, int size)
{
    socket_obj_t *s = container_of(base, socket_obj_t, base);
    if (type == SOCK_NOTFI_DATA_INCOMMING) {
        s->recv_incomming = 1;
        RIL_INFO("Data incomming.\r\n");
        s->unread_data_size = data ? (int)data : 0;
        return;
    } else if (type == SOCK_NOTFI_DATA_REPORT) {
        ril_sock_data_input(s, data, size);
        if (s->recv_event)                                //避免频繁产生接收事件
            return;
        else
            s->recv_event = true;
    } else
        RIL_INFO("Socket %d  notify [%d].\r\n", s->base.id, (int)type);

    async_work_add(&get_ril_obj()->workqueue, s, (void *)type, sock_notify_process);
}

/**
 * @brief  socket数据接收处理
 */
static void data_recv_proc(socket_obj_t *s)
{
    unsigned int buff_size, read_size;
    void *buff;
    if (!ril_sock_online(SOCK_FD(s)))
        return;                                           /* 未连接网络 */ 
    if (s->recv_incomming || s->unread_data_size) {        
        if (ring_buf_free_space(&s->rb) < 4)              /* 空间不足,暂时不读取 */
            return;
        buff_size = 1500;
        buff = ril_malloc(buff_size);
        if (buff == NULL)
            return;
        
        read_size = ril_dev_sock_recv(RIL_DEV, &s->base, buff, buff_size);
        if (read_size) {
            /* 上报数据 */
            ril_socket_notify(&s->base, SOCK_NOTFI_DATA_REPORT, buff, read_size);
            
            if (read_size > s->unread_data_size)
                s->unread_data_size -= read_size;
            else
                s->unread_data_size = 0;            
        } else {                                      /* 数据读取完毕 */
            s->recv_incomming   = 0; 
            s->unread_data_size = 0;
        }
        
        ril_free(buff);
    }
}

/**
 * @brief       socket状态监视
 * @param[in]   c
 * @return      none
 */
static void socket_status_watch(socket_obj_t *s)
{
    sock_request_status status;
    if (s->connstat == SOCK_STAT_BUSY) {                //连接状态查询
        if (ril_istimeout(s->conn_timer, MAX_SOCK_CONN_TIME * 1000)) {  
            update_sock_connstat(s, SOCK_STAT_TIMEOUT); //连接超时
        } else if (ril_istimeout(s->tick, 1000)) {
            s->tick = ril_get_ms();
            status = ril_dev_sock_conn_status(RIL_DEV, &s->base);
            if (status != SOCK_STAT_BUSY && status != SOCK_STAT_UNKNOW)
                update_sock_connstat(s, status);
        }
    }

    if (s->sendstat == SOCK_STAT_BUSY) {                //发送状态查询
        if (ril_istimeout(s->send_timer, MAX_SOCK_SEND_TIME * 1000)) {
            update_sock_sendstat(s, SOCK_STAT_TIMEOUT); //发送超时
        } else if (ril_istimeout(s->tick, 1000)) {
            s->tick = ril_get_ms();
            status = ril_dev_sock_send_status(RIL_DEV, &s->base);
            if (status != SOCK_STAT_BUSY && status != SOCK_STAT_UNKNOW) {
                update_sock_sendstat(s, status); 
            }
        }
    }    
}

/**
 * @brief  socket 初始化
 */
void ril_socket_init(void)
{
    INIT_LIST_HEAD(&sock_list);
}

/**
 * @brief  socket 监视任务
 */
void ril_socket_status_watch(void)
{
    socket_obj_t *s;
    struct list_head *list ,*n = NULL;     
    list_for_each_safe(list, n, &sock_list) {
        s = list_entry(list, socket_obj_t, node);    
        socket_status_watch(s);                        /* socket 状态管理*/
        data_recv_proc(s);                             /* socket 数据接收处理*/
    }
}
/**
 * @brief  强制清理socket资源
 */
void ril_sock_dispose(void)
{
    socket_obj_t *s;
    struct list_head *list ,*n = NULL;     
    
    ril_enter_critical();
    
    list_for_each_safe(list, n, &sock_list) {
        s = list_entry(list, socket_obj_t, node);
        
        s->sendstat = SOCK_STAT_UNKNOW;
        s->connstat = SOCK_STAT_UNKNOW;
        s->recv_incomming  = 1;
        s->conn_failed_cnt = 0;
        s->recv_event      = 0;
    }   
    
    ril_exit_critical();
}
