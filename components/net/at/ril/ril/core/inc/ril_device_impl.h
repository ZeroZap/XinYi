/******************************************************************************
 * @brief    ril 设备接口实现(Device interface implementation)
 * 
 * 
 *
 * 
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version. 
 ******************************************************************************/
#ifndef _RIL_DEV_IMPL_H_
#define _RIL_DEV_IMPL_H_

#include "ril_device.h"
#include "ril_device_common.h"
/**
 * @brief   设备启动
 */    
static inline int ril_dev_startup(struct ril_device *r)
{        
    return r->ops->startup ? r->ops->startup(r) : __ril_comm_startup(r);
}
/**
 * @brief   设备初始化
 */  
static inline int ril_dev_init(struct ril_device *r)
{
    return r->ops->init ? r->ops->init(r) : __ril_comm_init(r);
}
/**
 * @brief   设备关机
 */   
static inline int ril_dev_shutdown(struct ril_device *r)
{
    return r->ops->shutdown ? r->ops->shutdown(r) : __ril_comm_shutdown(r);
}
/**
 * @brief   复位设备
 */
static inline int ril_dev_reset(struct ril_device *r)
{
    return r->ops->reset ? r->ops->reset(r) : __ril_comm_reset(r);
}
/**
 * @brief         设备请求
 * @param[in/out] data - 参考ril_request_code描述
 * @param[in]     size - data大小
 */
static inline int ril_dev_request(struct ril_device *r, ril_request_code n, 
                                   void *data, int size)
{  
    return r->ops->request ? r->ops->request(r, n, data, size) : 
        __ril_comm_request(r, n, data, size);
}

/**
 * @brief   建立PDP环境
 */  
static inline int ril_dev_pdp_setup(struct ril_device *r)
{
    return r->ops->pdp_setup ? r->ops->pdp_setup(r) : __ril_comm_pdp_setup(r);    
}
                                   
/**
 * @brief     pdp控制
 * @param[in] active - 激活/去激活PDP
 */   
static inline int ril_dev_pdp_contrl(struct ril_device *r, bool active)
{
    return r->ops->pdp_contrl ? r->ops->pdp_contrl(r, active) : 
        __ril_comm_pdp_ctrl(r, active);     
}

/**
 * @brief     连接服务器
 */ 
static inline int ril_dev_sock_connect(struct ril_device *r, socket_base_t *s)
{
    if (r->ops->sock.connect)
        return r->ops->sock.connect(r, s);
    else
        return RIL_NOIMPL;
}
/**
 * @brief     发送数据
 */ 
static inline int ril_dev_sock_send(struct ril_device *r, socket_base_t *s,
                                     const void *buf, unsigned int len)
{
    if (r->ops->sock.send)
        return r->ops->sock.send(r, s, buf, len);
    else
        return RIL_NOIMPL;    
}
/**
 * @brief     接收数据
 */
static inline unsigned int ril_dev_sock_recv(struct ril_device *r, socket_base_t *s, 
                                             void *buf, unsigned int max_recv_size)
{
    if (r->ops->sock.recv)
        return r->ops->sock.recv(r, s, buf, max_recv_size);
    else
        return 0;
}
/**
 * @brief     断开服务器连接
 */ 
static inline int ril_dev_sock_disconnect(struct ril_device *r, socket_base_t *s)
{
    if (r->ops->sock.disconnect)
        return r->ops->sock.disconnect(r, s);
    else
        return RIL_NOIMPL;
}
/**
 * @brief     获取连接状态
 */
static inline sock_request_status ril_dev_sock_conn_status(struct ril_device *r, 
                                                           socket_base_t *s)
{
    if (r->ops->sock.conn_status)
        return r->ops->sock.conn_status(r, s);
    else
        return SOCK_STAT_UNKNOW;
}
/**
 * @brief     获取发送状态
 */ 
static inline sock_request_status ril_dev_sock_send_status(struct ril_device *r, 
                                                           socket_base_t *s)
{
    if (r->ops->sock.send_status)
        return r->ops->sock.send_status(r, s);
    else
        return SOCK_STAT_UNKNOW;      
}

#endif
