/******************************************************************************
 * @brief    基于gsm0710协议串口多路复用管理(multiplexing)
 *
 * Copyright (c) 2020, <master_roger@sina.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-05-27     Morro        Initial version. 
 ******************************************************************************/

#include "mux.h"
#include <string.h>
#include <stdio.h>

//格式：Flags(1B) Address(1B) Control(1B) Length(1~2B) Info(Length指定的可变长度) FCS(1B) Flag(1B)


#define BASIC_MODE_FLAG    0xF9          //基本模式标志位

#define MUX_EA_MASK   (1 << 0)           //EA位掩码
#define MUX_CR_MASK   (1 << 1)           //CR位掩码
#define MUX_PF_MASK   (1 << 4)           //PF位掩码


//reversed, 8-bit, poly=0x07 
const unsigned char crc_tbl[256] = { 
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 
    0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B, 
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 
    0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67, 
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 
    0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43, 
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 
    0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F, 
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 
    0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B, 
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 
    0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17, 
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 
    0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33, 
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 
    0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F, 
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 
    0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B, 
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 
    0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87, 
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 
    0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3, 
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 
    0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF, 
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 
    0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB, 
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 
    0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7, 
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 
    0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3, 
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 
    0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF 
};

/**
 * @brief	  mux帧校验
 * @retval    none
 */
static unsigned char frame_check(const unsigned char *buf, int count) {
    unsigned char fcs = 0xFF;
    int i;
    for (i = 0; i < count; i++) {
        fcs = crc_tbl[fcs^buf[i]];
    }
    return (0xFF - fcs);
}

/**
 * @brief	   发送帧
 * @param[in]  channel - 通道号
 * @param[in]  type    - 帧类型
 * @param[in]  buf     - 数据缓冲区
 * @param[in]  size    - 缓冲区长度
 * @return 	   none
 */
bool mux_send_frame(mux_obj_t *obj, int channel, unsigned char type, 
                    const char *buf, int size)
{
	//帧头
	unsigned char header[5];;     
    //帧尾
	unsigned char tail[2] = { 0xFF, BASIC_MODE_FLAG };                  
	int cnt;
    header[0] = BASIC_MODE_FLAG;
	// 地址域,EA=1
	header[1] = MUX_EA_MASK | MUX_CR_MASK | (channel << 2);
	//控制域
	header[2] = type | MUX_PF_MASK;

    if (size > 127) {                               
        cnt = 5;
        header[3] = (unsigned char )(size << 1);
        header[4] = (unsigned char )(size >> 7);       
    } else {
        header[3] = (size << 1) | MUX_EA_MASK;
        cnt = 4;
    }
	tail[0] = frame_check(header + 1, cnt - 1);        //生成校验码       
	if (obj->adt->write(header, cnt) != cnt) {         //发送头
		return false;
	}
	if (obj->adt->write(buf, size) != size) {
		return false;
	}
	return obj->adt->write(tail, 2) == 2;
}


/**
 * @brief	  mux协议帧处理[F9 07 73 01 15 F9]
 * @retval    none
 */
void mux_process(mux_obj_t *obj)
{
    unsigned char channel, type, crc;
    int i, frame_len;
    if (obj->state) {
        if ((obj->adt->get_tick() - obj->timer) > 5000) {          //超时处理
            obj->state   = 0;
            obj->recvcnt = 0;
            CMUX_DBG("recv timeout\r\n");
        }
    }
    obj->recvcnt += obj->adt->read(&obj->data[obj->recvcnt], 
                                   sizeof(obj->data) - obj->recvcnt);
    //状态0->帖起始标志     
    if (obj->state == 0) {                                                                           
        for (i = 0; i < obj->recvcnt; i++) {            
            if (obj->data[i] == BASIC_MODE_FLAG && obj->data[i + 1] != obj->data[i]) {
                obj->recvcnt -= i;
                /*剩余数据移动到头部*/
                memmove(obj->data, &obj->data[i], obj->recvcnt);
                obj->timer  = obj->adt->get_tick();
                obj->state  = 1;   
                obj->len    = 0;
                return;
            }
        }
    }
    if (obj->state != 1 || obj->recvcnt < 5)
        return;
    //状态1->解析长度 
    if (obj->state != 2) {                                                
        obj->len = obj->data[3] >> 1;                           //获取数据域长度
        obj->offset = 4;
        if (!(obj->data[3] & MUX_EA_MASK)) {                    //双字节长度域
            obj->len |= obj->data[4] << 7;
            obj->offset++;
        }
        obj->state = 2;
    }
    frame_len = obj->len + obj->offset + 2;                     //当前帧总长度
    if (obj->state == 2 && obj->recvcnt >= frame_len) {
        crc = obj->data[frame_len - 2];
        if (crc == frame_check(&obj->data[1], obj->offset - 1)) {
            channel = obj->data[1] >> 2 ;                       //通道号
            type = obj->data[2] & (~MUX_PF_MASK);               //类型
            //CMUX_DBG("channel:%d, type:%d\r\n", channel, type);
            obj->adt->recvEvent(channel, type, &obj->data[obj->offset ], obj->len);
        } else
            CMUX_DBG("check failed.\r\n");
        
        obj->recvcnt -= frame_len;                              //剩余未解析数据
        /*移动下一帧剩余数据到前面*/
        memmove(obj->data, &obj->data[frame_len], obj->recvcnt);
        obj->state   = 0;                                     //返回初始状态
    }             
}

/**
 * @brief     打开通道
 */
bool mux_open_channel(mux_obj_t *obj, int channel)
{
    return mux_send_frame(obj, channel, MUX_SABM, 0, 0);
}

/**
 * @brief     关闭通道
 */
bool mux_close_channel(mux_obj_t *obj, int channel)
{
    return mux_send_frame(obj, channel, MUX_DISC, 0, 0);
}

/**
 * @brief	  mux对象初始化
 * @retval    none
 */
void mux_init(mux_obj_t *obj, const mux_adater_t *adt)
{
    obj->state = 0;
    obj->adt = adt;
}
