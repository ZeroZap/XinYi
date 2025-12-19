/******************************************************************************
 * @brief    HTTP 客户端管理
 *
 * Copyright (c) 2021 <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-20     Morro        Initial version
 * 2021-12-08     Morro        Fix the problem of HTTP download data offset by 2 bytes.
 ******************************************************************************/
#include "ril.h"
#include "ril_socket.h"
#include "comdef.h"
#include "http_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*HTTP信息*/
typedef struct {
    ril_socket_t   socket;   
    http_client_t  client;
    bool           abort;
    char           host[128];                           // 远程主机名称
    const char     *path;                               // 文件路径
    unsigned short port;                                // 远程端口    
    unsigned short max_timeout;                         // 最大超时时间s  
    unsigned char  state;
    unsigned char  errcnt;                              //异常计数
    unsigned int   wait_timer;
    unsigned int   timer;                               //超时定时器
    unsigned int   conn_timer;                          //连续超时   
    unsigned int   total_bytes;                         //总字节数
    unsigned int   recv_bytes;                          //已接收字节数    
    unsigned int   speed;                               //平均下载速度
    char           buf[MAX_HTTPBUF_SIZE];
}http_info_t;

/*http 响应头 ----------------------------------------------------------------*/
typedef struct {
    int status;                                         //状态码
    int range_from, range_to;                           //本次传输文件开始-结束位置
    unsigned int content_size;                          //本次传输内容大小
    unsigned int total_size;                            //总文件大小
}http_header_t;

//下载超时判断
bool is_download_timeout(http_info_t *info)
{
    return ril_istimeout(info->timer, info->max_timeout * 1000);
}
//连接超时判断
bool is_connection_timeout(http_info_t *info)
{
    return ril_istimeout(info->conn_timer, 120 * 1000);
}
/**
 * @brief       解析http头
 */
bool parse_http_header(char *buf, http_header_t *h)
{
    char *start;
    int total;
    /*+------------一般HTTP请求得到的响应头部信息如下---------------------------
    HTTP/1.1 200 OK      (CR LF)      -第一行为http版本信息,响应状态码
    Server: xxxx         (CR LF)   
    Connection: xxxxx    (CR LF)
    Content-Type:xxxx    (CR LF)   
    Content-Length: N    (CR LF)     -数据长度N
    xxxxx:xxxx
    (CR LF)                          -头部信息与数据域间以两个回车换行隔开
    (CR LF)
    data[0..N-1]                     -数据域(长度由Content-Length决定)
     ------------------------------------------------------------------------+*/    
    start = strstr(buf, "HTTP/");                      //http开头
    if (start == NULL)
        goto error;
    if (sscanf(start, "%*[^ ] %d", &h->status) != 1) {
        goto error;
    }
    if ((start = strstr(buf, "Content-Length: ")) == NULL)
        goto error;
    start += 16;
    h->content_size = atoi(start);

    if ((start = strstr(buf, "Content-Range: ")) == NULL)
        goto error;    
    start += 15;
    if (sscanf(start, "bytes %d-%d/%d", 
               &h->range_from, &h->range_to, &total) != 3) {
        goto error;
    }
    h->total_size = total;
    return true;
error:
    HTTP_DBG("header error...%s\r\n", buf);
    return false;
}


/**
 * @brief       等待HTTP应答
 * @param[in]   state - 当前状态
 * @retval      buf   - 
 */
static void onDataRecv(http_info_t *info, int state, void *buf, unsigned int size)
{
    http_event_args_t e;
    e.client  = &info->client;
    e.state   = state;
    e.data    = buf;
    e.datalen = size;
    e.filesize= info->total_bytes;
    e.offset  = info->recv_bytes;
    e.spand_time = ril_get_ms() - info->timer;
    
    if (info->recv_bytes + size <= info->total_bytes) {
        info->client.event(&e);  
    }
    info->recv_bytes += size;
}

/**
 * @brief       等待HTTP应答
 * @param[in]   range_from/to - 数据起、止位置
 * @retval      RIL_OK - 无错误
 */
static int wait_http_resp(http_info_t *info)
{
    char *start;
    http_header_t hdr;    
    int read_size;                                      //当前读取大小
    int read_cnt;                                       //缓冲区有效数据长度
    int content_cnt = 0;                                //实际有效数据计数   
    bool find_header;                                   //HTTP头解析标志
    unsigned int timer;                                 //超时定时器    
    read_cnt = 0;
    find_header = false;
    timer = ril_get_ms();
    while (!ril_istimeout(timer, MAX_RECV_TIMEOUT * 1000) && !info->abort &&
           ril_sock_online(info->socket)) {  
        
        read_size = ril_sock_recv(info->socket, &info->buf[read_cnt], 
                                  sizeof(info->buf) - read_cnt - 1);
        if (read_size == 0) {
            ril_delay(20);
            continue;
        }
        read_cnt += read_size;
        timer = ril_get_ms();                           //收到数据, 重置定时器
        if (find_header) {
            content_cnt += read_size;                   //累计收到的内容长度
            if (read_cnt >= MAX_HTTPBUF_SIZE / 4) {      //读取一定量再写入
                onDataRecv(info, HTTP_STAT_DATA, info->buf, read_cnt);
                read_cnt = 0;
            }
            if (content_cnt >= hdr.content_size) {      //内容读取完毕
                if (read_cnt) 
                    onDataRecv(info, HTTP_STAT_DATA, info->buf, read_cnt);   
                return RIL_OK;
            }
        } else if (!find_header && read_cnt > 32) {
            info->buf[read_cnt] = '\0';
            if ((start = strstr(info->buf, "\r\n\r\n")) == NULL)
                continue;
            if (!parse_http_header(info->buf, &hdr)) {  //解析HTTP响应头
                HTTP_DBG("header parse error\r\n");
                return RIL_ERROR;
            }            
            if (hdr.status != 206 && hdr.status != 200 ||
                hdr.total_size != info->total_bytes) {   //文件发生改变 
                return RIL_ERROR;
            }
            
            find_header = true;                         
            read_cnt -= start + 4 - info->buf;            //除去头后实际有效长度            
            onDataRecv(info, HTTP_STAT_DATA, start + 4, read_cnt);           
            content_cnt = read_cnt;
            read_cnt = 0;
            
            if (content_cnt >= hdr.content_size)          //1包就结束
                return RIL_OK;            
        }
    }
    
    /* 接收超时处理*/
    if (read_cnt)             
        onDataRecv(info, HTTP_STAT_DATA, info->buf, read_cnt);
    
    return RIL_TIMEOUT;

}

/**
 * @brief      创建http请求头
 * @param[in]  buf        - 请求头存储缓冲区
 * @param[in]  bufsize    - 缓冲区大小
 * @param[in]  host       - 远程主机
 * @param[in]  path       - 文件路径
 * @param[in]  range_from - 请求的文件开始位置
 * @param[in]  range_to   - 请求的文件结束位置
 * 
 * @retval     size
 */
static int create_http_header(char *buf, int bufsize, const char *host, 
                              const char *path, int range_from, int range_to)
{
    return snprintf(buf, bufsize, 
                    "GET %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Accept: */* Accept-Language: en-us,en-gb,zh-cn\r\n"
                    "Keep-Alive: timeout=20\r\n"
                    "Range: bytes=%d-%d\r\n"
                    "Connection: Keep-Alive\r\n\r\n", path, host, 
                    range_from, range_to);
}

/**
 * @brief      获取文件信息
 * @param[in]  header    - HTTP头
 */
static int get_file_info(http_info_t *info, http_header_t *header)
{
    int size;       
    int recvcnt = 0;
    unsigned int timer;                                        //超时定时器
    size = create_http_header(info->buf, sizeof(info->buf), info->host, 
                              info->path, 0, 1);
    HTTP_DBG("Get file infomation...\r\n");
    if (ril_sock_send(info->socket, info->buf, size) != RIL_OK)  /*发送请求*/
        return RIL_ERROR;
    timer = ril_get_ms();
    
    while (!ril_istimeout(timer, MAX_RECV_TIMEOUT * 1000) && !info->abort) {
        size = ril_sock_recv(info->socket, &info->buf[recvcnt], 
                             sizeof(info->buf) - recvcnt - 1);
        if (size == 0) {
            ril_delay(50);
            continue;
        }         
        recvcnt += size;
        
        info->buf[recvcnt] = '\0';
        if (!strstr(info->buf, "\r\n\r\n"))
            continue;
        if (!parse_http_header(info->buf, header) ||
            (header->status != 206 && header->status != 200) ) {
                HTTP_DBG("%s\r\n", info->buf);
                return RIL_ERROR;
            }
        else
            return RIL_OK;
    }

    return info->abort ? RIL_ABORT : RIL_TIMEOUT;
}

/**
 * @brief  获取文件数据
 * @param[in]  range_from - 文件起始偏移
 * @param[in]  range_from - 文件截止偏移     
 */
static int request_data(http_info_t *info, int range_from, int range_to)
{
    int size;
    size = create_http_header(info->buf, sizeof(info->buf), info->host, info->path, 
                              range_from, range_to);
    HTTP_DBG("Send data request:\r\n%s\r\n", info->buf);
    //发送请求
    if (ril_sock_send_async(info->socket, info->buf, size) != RIL_OK)
        return RIL_ERROR;
    else
        return wait_http_resp(info);
}

/**
 * @brief  错误处理
 */
static void error_process(http_info_t *info)
{
    info->wait_timer = ril_get_ms();
    info->errcnt++;
    if (ril_sock_online(info->socket))
        ril_sock_disconnect(info->socket);
}

/**
 * @brief  判断是否产生异常
 */
static int is_error_occur(http_info_t *info)
{
    return info->errcnt > 3 || is_connection_timeout(info);
}

static int connect_to_server(http_info_t *info)
{
    HTTP_DBG("Connect to server[host:%s, port:%d]\r\n", info->host, info->port);      
    return ril_sock_connect(info->socket, info->host, info->port,RIL_SOCK_TCP);         
}

/**
 * @brief  文件下载
 */
static int download_file(http_info_t *info)
{
    http_header_t h;
    int blksize, remain;
    unsigned int timer = 0;
    int ret;
    onDataRecv(info, HTTP_STAT_START, info->buf, 0); 
    while (!is_download_timeout(info) && !info->abort ) {
        if (is_error_occur(info))                   //异常退出
            return RIL_FAILED;                 
        ril_delay(100);
        if (!ril_isonline() || !ril_istimeout(timer, info->errcnt * info->errcnt * 2000))
            continue;
        /* 连接服务器 ---------------------------------------------------------*/
        if (!ril_sock_online(info->socket) && connect_to_server(info) != RIL_OK) {
            error_process(info);
            continue;
        } else
            info->conn_timer = ril_get_ms();
        
        if (info->state == 0) {
            ret = get_file_info(info, &h);                //获取文件信息
            if ((ret != RIL_OK)) {
                error_process(info);
            } else {
                info->state       = 1;
                info->total_bytes = h.total_size;
                info->recv_bytes  = 0;
                info->errcnt      = 0;
                HTTP_DBG("File Size:%d bytes\r\n", info->total_bytes);
            }
        } else {
            remain = info->total_bytes - info->recv_bytes;//剩余未下载数据长度  
            //下面进行分块下载并存储,每次最大50k
            blksize = remain > MAX_HTTP_REQUEST_SIZE ? MAX_HTTP_REQUEST_SIZE : remain;
            ret = request_data(info, info->recv_bytes, info->recv_bytes + blksize - 1);
            /* 下载异常处理  -----------------------------------------------------*/
            if (ret != RIL_OK) {
               error_process(info); 
            } else if (info->recv_bytes >= info->total_bytes) {//下载完成
                onDataRecv(info, HTTP_STAT_DONE, info->buf, 0); 
                return RIL_OK;
            } else {
                info->errcnt = 0;
            }
        }
       timer = ril_get_ms();
    }
    onDataRecv(info, HTTP_STAT_FAILED, info->buf, 0); 
    HTTP_DBG("Download failed\r\n");
    return info->abort ? RIL_ABORT : RIL_TIMEOUT;
}

/**
 * @brief      创建http客户端
 * @param[in]  e    - 事件处理接口
 * @param[in]  host - 主机地址(www.xxx.com)
 * @param[in]  port - 端口(一般填80)
 * @return     NULL - 创建失败, 其它值 - http客户端
 */
http_client_t *http_client_create(http_event_t e, const char *host, 
                                  unsigned short port)
{
    http_info_t *info;
    info = (http_info_t *)ril_malloc(sizeof(http_info_t));
    
    if (info == NULL)
        return NULL;
    memset(info, 0, sizeof(http_info_t));    
    info->client.event = e;
    snprintf(info->host, sizeof(info->host), "%s", host);
    info->port = port;
    return &info->client;
}
/**
 * @brief      销毁http客户端
 */
void http_client_destroy(http_client_t *hc)
{       
    http_info_t *info = container_of(hc, http_info_t, client);
    ril_free(info);
}

/**
 * @brief      启动HTTP下载
 * @param[in]  hc      -  http客户端
 * @param[in]  file    -  下载文件名称(如/demo.hex)
 * @param[in]  timeout -  下载超时时间(ms)
 * @return     RIL_OK - 下载成功, 其它值 - 下载失败
 */
int http_start_download(http_client_t *hc, const char *file, unsigned int timeout)
{
    int ret = RIL_ERROR;
    http_info_t *info = container_of(hc, http_info_t, client);
    HTTP_DBG("Start download file:%s\r\n", file);
    info->socket = ril_sock_create(NULL, 2048);
    if (info->socket <= 0) {
        return RIL_NOMEM;
    }
    info->abort       = false;
    info->max_timeout = timeout;
    info->conn_timer  = info->timer = ril_get_ms();            //超时定时器    
    info->path        = file;
    ret = download_file(info);
    //释放资源
    ril_sock_disconnect(info->socket);
    ril_sock_destroy(info->socket);

    return ret;
}

/**
 * @brief  停止HTTP升级
 */
void http_stop_download(http_client_t *hc) 
{
    container_of(hc, http_info_t, client)->abort = true;
}
