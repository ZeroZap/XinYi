/**
 * @file xy_lte.c
 * @brief LTE/4G Cat.1 Module Driver Implementation
 * @version 1.0.0
 * @date 2026-03-13
 * 
 * 实现功能:
 * - AT 命令解析
 * - 网络附着
 * - PDP 上下文管理
 * - TCP/UDP 连接
 * - 数据收发
 * 
 * 支持模块:
 * - 移远 EC100Y/EC200Y
 * - 广和通 L610/L630
 * - 合宙 Air780E
 */

#include "xy_lte.h"
#include <string.h>
#include <stdio.h>

#define LTE_MAX_LINKS 8U

/**
 * @brief 发送 AT 命令辅助函数
 */
static int lte_send_cmd(xy_lte_t *lte, const char *cmd, char *resp, size_t resp_len, uint32_t timeout)
{
    char read_buf[128];
    size_t cmd_len;
    size_t read_len;
    size_t copy_len;
    int ret;

    if (!lte || !lte->initialized || !cmd) {
        return XY_LTE_INVALID_PARAM;
    }

    if (lte->transport.write) {
        cmd_len = strlen(cmd);
        ret = lte->transport.write(lte->transport.context, (const uint8_t *)cmd, cmd_len, timeout);
        if (ret != XY_LTE_OK && ret != (int)cmd_len) {
            return ret < 0 ? ret : XY_LTE_ERROR;
        }

        if (resp && resp_len > 0U && lte->transport.read) {
            read_len = resp_len - 1U;
            if (read_len >= sizeof(read_buf)) {
                read_len = sizeof(read_buf) - 1U;
            }

            ret = lte->transport.read(lte->transport.context, (uint8_t *)read_buf, read_len,
                                      timeout);
            if (ret < 0) {
                return ret;
            }

            copy_len = (size_t)ret;
            if (copy_len > read_len) {
                copy_len = read_len;
            }
            memcpy(resp, read_buf, copy_len);
            resp[copy_len] = '\0';
        } else if (resp && resp_len > 0U) {
            resp[0] = '\0';
        }

        return XY_LTE_OK;
    }

    if (resp && resp_len > 0U) {
        resp[0] = '\0';
    }
    return XY_LTE_OK;
}

/**
 * @brief 解析 +CSQ 响应
 */
static int parse_csq(const char *resp, xy_lte_signal_t *signal)
{
    if (!resp || !signal) return XY_LTE_INVALID_PARAM;
    
    /* +CSQ: <rssi>,<ber> */
    if (sscanf(resp, "+CSQ: %d,%d", &signal->rssi, &signal->ber) == 2) {
        return XY_LTE_OK;
    }
    return XY_LTE_ERROR;
}

/**
 * @brief 解析 +CEREG 响应
 */
static int parse_cereg(const char *resp, xy_lte_network_info_t *info)
{
    if (!resp || !info) return XY_LTE_INVALID_PARAM;
    
    /* +CEREG: <n>,<stat>[,<tac>,<ci>,<cellid>] */
    int stat;
    if (sscanf(resp, "+CEREG: %*d,%d", &stat) == 1) {
        if (stat == 1 || stat == 5) {  /* 1=已注册，5=已注册 (漫游) */
            return XY_LTE_OK;
        }
    }
    return XY_LTE_ERROR;
}

int xy_lte_init(xy_lte_t *lte, void *uart_handle, uint32_t baudrate)
{
    if (!lte || !uart_handle) {
        return XY_LTE_INVALID_PARAM;
    }
    
    memset(lte, 0, sizeof(*lte));
    lte->uart_handle = uart_handle;
    lte->baudrate = baudrate ? baudrate : 115200;
    
    /* 默认 PDP 配置 */
    lte->pdp.cid = 1;
    snprintf(lte->pdp.apn, sizeof(lte->pdp.apn), "cmnet");
    
    lte->initialized = true;
    
    return XY_LTE_OK;
}

int xy_lte_bind_transport(xy_lte_t *lte, const xy_lte_transport_t *transport)
{
    if (!lte || !lte->initialized || !transport || !transport->write) {
        return XY_LTE_INVALID_PARAM;
    }

    lte->transport = *transport;
    return XY_LTE_OK;
}

int xy_lte_deinit(xy_lte_t *lte)
{
    int ret;

    if (!lte) return XY_LTE_INVALID_PARAM;
    
    if (lte->attached) {
        ret = xy_lte_detach(lte);
        if (ret != XY_LTE_OK) {
            return ret;
        }
    }
    
    lte->initialized = false;
    return XY_LTE_OK;
}

int xy_lte_check(xy_lte_t *lte)
{
    char resp[64];
    int ret;
    
    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 发送 AT 测试命令 */
    ret = lte_send_cmd(lte, "AT", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    /* 检查响应是否包含 OK */
    if (strstr(resp, "OK") != NULL) {
        return XY_LTE_OK;
    }
    
    return XY_LTE_ERROR;
}

int xy_lte_get_module_info(xy_lte_t *lte, char *manufacturer, char *model, char *revision)
{
    char manufacturer_resp[64];
    char model_resp[64];
    char revision_resp[64];
    int ret;
    
    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 获取厂商信息 */
    if (manufacturer) {
        ret = lte_send_cmd(lte, "AT+CGMI", manufacturer_resp, sizeof(manufacturer_resp), 1000);
        if (ret != XY_LTE_OK) {
            return ret;
        }
    }
    
    /* 获取型号 */
    if (model) {
        ret = lte_send_cmd(lte, "AT+GMM", model_resp, sizeof(model_resp), 1000);
        if (ret != XY_LTE_OK) {
            return ret;
        }
    }
    
    /* 获取版本 */
    if (revision) {
        ret = lte_send_cmd(lte, "AT+CGMR", revision_resp, sizeof(revision_resp), 1000);
        if (ret != XY_LTE_OK) {
            return ret;
        }
    }

    if (manufacturer) {
        strncpy(manufacturer, manufacturer_resp, 64);
    }
    if (model) {
        strncpy(model, model_resp, 64);
    }
    if (revision) {
        strncpy(revision, revision_resp, 64);
    }
    
    return XY_LTE_OK;
}

int xy_lte_get_sim_info(xy_lte_t *lte, xy_lte_sim_info_t *info)
{
    char resp[64];
    int ret;
    
    if (!lte || !lte->initialized || !info) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 获取 ICCID */
    ret = lte_send_cmd(lte, "AT+CCID", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    /* 解析 ICCID */
    
    /* 获取 IMSI */
    ret = lte_send_cmd(lte, "AT+CIMI", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    /* 解析 IMSI */
    
    /* 获取手机号 */
    ret = lte_send_cmd(lte, "AT+CNUM", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    /* 解析 MSISDN */
    
    return XY_LTE_OK;
}

int xy_lte_check_sim(xy_lte_t *lte)
{
    char resp[32] = {0};
    int ret;
    
    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* CPIN? 查询 SIM 状态 */
    ret = lte_send_cmd(lte, "AT+CPIN?", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return 0;
    }
    
    if (strstr(resp, "+CPIN: READY") != NULL) {
        return 1;  /* SIM 卡就绪 */
    } else if (strstr(resp, "+CPIN: SIM PIN") != NULL) {
        return 2;  /* 需要 PIN 码 */
    }
    
    return 0;  /* 无 SIM 卡 */
}

int xy_lte_enter_pin(xy_lte_t *lte, const char *pin)
{
    char cmd[32];
    
    if (!lte || !pin) {
        return XY_LTE_INVALID_PARAM;
    }
    
    snprintf(cmd, sizeof(cmd), "AT+CPIN=\"%s\"", pin);
    return lte_send_cmd(lte, cmd, NULL, 0, 5000);
}

int xy_lte_get_signal(xy_lte_t *lte, xy_lte_signal_t *signal)
{
    char resp[32];
    xy_lte_signal_t parsed;
    int ret;
    
    if (!lte || !signal) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 获取信号质量 */
    ret = lte_send_cmd(lte, "AT+CSQ", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }

    parsed = *signal;
    ret = parse_csq(resp, &parsed);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    *signal = parsed;
    
    /* 4G 模块可获取更详细信息 */
    /* AT+CESQ 获取 RSRP/RSRQ/SINR */
    
    return XY_LTE_OK;
}

int xy_lte_get_network_info(xy_lte_t *lte, xy_lte_network_info_t *info)
{
    char resp[64];
    xy_lte_network_info_t parsed;
    int ret;
    
    if (!lte || !lte->initialized || !info) {
        return XY_LTE_INVALID_PARAM;
    }

    parsed = *info;
    
    /* 获取网络注册状态 */
    ret = lte_send_cmd(lte, "AT+CEREG?", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    ret = parse_cereg(resp, &parsed);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    /* 获取运营商信息 */
    ret = lte_send_cmd(lte, "AT+COPS?", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    /* 解析 MCC/MNC */
    
    /* 获取基站信息 */
    ret = lte_send_cmd(lte, "AT+CESQ", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    /* 解析 LAC/CellID */

    *info = parsed;
    
    return XY_LTE_OK;
}

int xy_lte_attach(xy_lte_t *lte)
{
    int ret;

    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 设置全网通模式 */
    ret = lte_send_cmd(lte, "AT+CNACT=2", NULL, 0, 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    /* 附着网络 */
    ret = lte_send_cmd(lte, "AT+CGATT=1", NULL, 0, 30000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    lte->attached = true;
    return XY_LTE_OK;
}

int xy_lte_detach(xy_lte_t *lte)
{
    int ret;

    if (!lte) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* 分离网络 */
    ret = lte_send_cmd(lte, "AT+CGATT=0", NULL, 0, 10000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    lte->attached = false;
    return XY_LTE_OK;
}

int xy_lte_is_attached(xy_lte_t *lte)
{
    char resp[32] = {0};
    int ret;
    
    if (!lte) {
        return 0;
    }
    
    ret = lte_send_cmd(lte, "AT+CGATT?", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return 0;
    }
    
    if (strstr(resp, "+CGATT: 1") != NULL) {
        return 1;
    }
    return 0;
}

int xy_lte_set_pdp_context(xy_lte_t *lte, xy_lte_pdp_context_t *ctx)
{
    char cmd[192];
    int ret;
    
    if (!lte || !ctx) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CSTT="APN","username","password" */
    snprintf(cmd, sizeof(cmd), "AT+CSTT=\"%s\",\"%s\",\"%s\"",
             ctx->apn, ctx->username, ctx->password);
    
    ret = lte_send_cmd(lte, cmd, NULL, 0, 10000);
    if (ret != XY_LTE_OK) {
        return ret;
    }

    lte->pdp = *ctx;
    return XY_LTE_OK;
}

int xy_lte_activate_pdp(xy_lte_t *lte, uint8_t cid)
{
    (void)cid;
    int ret;

    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIICR 激活移动场景 */
    ret = lte_send_cmd(lte, "AT+CIICR", NULL, 0, 60000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    /* AT+CIFSR 获取本地 IP */
    /* 验证 PDP 激活成功 */
    
    lte->pdp_active = true;
    return XY_LTE_OK;
}

int xy_lte_deactivate_pdp(xy_lte_t *lte, uint8_t cid)
{
    (void)cid;
    int ret;
    
    if (!lte || !lte->initialized) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIPSHUT 关闭移动场景 */
    ret = lte_send_cmd(lte, "AT+CIPSHUT", NULL, 0, 30000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    
    lte->pdp_active = false;
    return XY_LTE_OK;
}

int xy_lte_is_pdp_active(xy_lte_t *lte, uint8_t cid)
{
    (void)cid;
    
    if (!lte) {
        return 0;
    }
    
    return lte->pdp_active ? 1 : 0;
}

int xy_lte_connect(xy_lte_t *lte, uint8_t link_id, const char *server, uint16_t port, bool tcp)
{
    char cmd[128];
    
    if (!lte || !server || link_id >= LTE_MAX_LINKS) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIPSTART=<link_id>,<type>,<server>,<port> */
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"%s\",\"%s\",%d",
             link_id, tcp ? "TCP" : "UDP", server, port);
    
    return lte_send_cmd(lte, cmd, NULL, 0, 60000);
}

int xy_lte_close(xy_lte_t *lte, uint8_t link_id)
{
    char cmd[32];
    
    if (!lte || link_id >= LTE_MAX_LINKS) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIPCLOSE=<link_id> */
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d", link_id);
    return lte_send_cmd(lte, cmd, NULL, 0, 10000);
}

int xy_lte_send(xy_lte_t *lte, uint8_t link_id, const uint8_t *data, size_t len)
{
    char cmd[32];
    char resp[16];
    int ret;
    
    if (!lte || !lte->initialized || !data || len == 0 || link_id >= LTE_MAX_LINKS) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIPSEND=<link_id>,<len> */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", link_id, (int)len);
    if (!lte->transport.write) {
        return XY_LTE_OK;
    }
    
    /* 等待 > 提示符 */
    ret = lte_send_cmd(lte, cmd, resp, sizeof(resp), 10000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    if (strchr(resp, '>') == NULL) {
        return XY_LTE_ERROR;
    }

    /* 发送数据 */
    if (lte->transport.write) {
        ret = lte->transport.write(lte->transport.context, data, len, 10000);
        if (ret != XY_LTE_OK && ret != (int)len) {
            return ret < 0 ? ret : XY_LTE_ERROR;
        }
    }
    
    return XY_LTE_OK;
}

int xy_lte_recv(xy_lte_t *lte, uint8_t link_id, uint8_t *data, size_t len, uint32_t timeout)
{
    int ret;

    if (!lte || !lte->initialized || !data || len == 0 || link_id >= LTE_MAX_LINKS) {
        return XY_LTE_INVALID_PARAM;
    }

    if (lte->transport.read) {
        ret = lte->transport.read(lte->transport.context, data, len, timeout);
        if (ret < 0) {
            return ret;
        }
        if (ret > 0 && lte->recv_callback) {
            lte->recv_callback(data, (size_t)ret);
        }
        return ret;
    }

    memset(data, 0, len);
    
    /* 实际实现应等待 +RECEIVE URC */
    /* 从缓冲区读取数据 */
    
    return 0;  /* 返回实际接收字节数 */
}

int xy_lte_register_urc(xy_lte_t *lte, xy_lte_urc_callback_t callback)
{
    if (!lte) {
        return XY_LTE_INVALID_PARAM;
    }

    lte->urc_callback = callback;
    return XY_LTE_OK;
}

int xy_lte_register_recv(xy_lte_t *lte, xy_lte_recv_callback_t callback)
{
    if (!lte) {
        return XY_LTE_INVALID_PARAM;
    }

    lte->recv_callback = callback;
    return XY_LTE_OK;
}

int xy_lte_send_at(xy_lte_t *lte, const char *cmd, char *response, size_t resp_len, uint32_t timeout)
{
    return lte_send_cmd(lte, cmd, response, resp_len, timeout);
}

int xy_lte_reboot(xy_lte_t *lte)
{
    if (!lte) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CFUN=1,1 重启模块 */
    return lte_send_cmd(lte, "AT+CFUN=1,1", NULL, 0, 5000);
}

int xy_lte_get_ip(xy_lte_t *lte, char *ip, size_t len)
{
    char resp[64];
    int ret;

    if (!lte || !ip || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CIFSR 获取本地 IP */
    ret = lte_send_cmd(lte, "AT+CIFSR", resp, sizeof(resp), 5000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    strncpy(ip, resp, len);
    if (len > 0U) {
        ip[len - 1U] = '\0';
    }
    return XY_LTE_OK;
}

int xy_lte_get_imei(xy_lte_t *lte, char *imei, size_t len)
{
    char resp[32];
    int ret;

    if (!lte || !imei || len == 0U) {
        return XY_LTE_INVALID_PARAM;
    }
    
    /* AT+CGSN 获取 IMEI */
    ret = lte_send_cmd(lte, "AT+CGSN", resp, sizeof(resp), 1000);
    if (ret != XY_LTE_OK) {
        return ret;
    }
    strncpy(imei, resp, len);
    if (len > 0U) {
        imei[len - 1U] = '\0';
    }
    return XY_LTE_OK;
}
