## at 核心处理
### 行数据读取
- 清空 当前行获取的字符数为 0
- 清空 client->recv_line_buf
- 不断从串口 ringbuffer 接收数据
- 判断当前接收数据是否已经满了
- 判断是否进入循环处理按下述或条件
  - 如果有 urc 处理对象 get_urc_obj
    - 获取 urc table 的前缀后缀匹配
    - 判断当前是否有满足条件的 urc 处理
    - 如果满足就返回对应的 urc 处理，也就是 urc_pbj

  - 如果当前读取的字符为 \n 且之前最后一个字符为 \r
  - 如果 end_sign != 0 且当前 char 等于 client-> endsign
- 判断 full，
  - 如果 full 则返回 full error
  - 如果不为 full 则break往下执行
- 返回 当前行接收到的数据 client->recv_line_len

### 如果读取到行数据了
- 读取一个字符（uart 的 ring buffer 处理）
- if URC 优先处理  进一步解析
- else reponse 处理： 如果不是 URC， resp 又不为 空，则进一步去获取 line 数据 且且且更新 response 结果
  - 将 \n 转换 成 \0
  - 如果当前resp->buf_len + client->recv_line_len < client->buff_max_size;
    -  resp_buf_len += recv_line_len
    - resp->line_count++;
  - 如果 clinet->end_sign !=0 且 end_ch = client->end_sign 且 又 resp->line_num == 0
    - 如果有指定 cline->end_sigh，上面的 uart 抓取到对应的字符，则也可以返回 OK
    - 如果 resp->line_num == 0 表示等待 OK，且 end_ch 又符合设定
    - TODO 上述，确认该问题，之前不是抹掉，
    - 返回 response OK
  - 如果抓取到 OK，也 reponse ok
  - 如果抓取到 其他指定的，也返回 OK
  - 如果抓到 ERROR 则返回 ERROR
  - 如果抓取到指定行数， 也返回 OK
  - - client->resp_status = AT_RESP_OK
  - !!!!!!!!!!!!!!!!!  resp->notice处理
- else

## 命令发送 at_obj_exec_cmd
- 清除 resp->buf_len, line_counts
- 上锁
- client->resp_status = AT_RESP_OK; 写状态
- rt_sem_control， 把 resp->notice 清零
- at_vprintfln !!! 发送 AT 指令！！！！！！！
- take resp->notice 外部发送命令后任务处理解析数据，
  - 如果长时还是没接收到 resp->notice ，则认为超时了
  - 如果等到了结果且又不是 OK 的，就设定为 ERROR? 还有其他状态？？
- 释放 client->resp = RT_NULL;
- 释放锁

## 命令发送后的处理
- 获取 resp buffer 抓取想要的数据

## 进入透传模式
- 发送命令

## 透传模式收发

## 退出透传模式
- 多久不干事，那就退出了