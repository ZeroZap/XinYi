
## ref
<https://doc.embedfire.com/mcu/stm32/f103mini/std/zh/latest/book/SDcard.html>
<https://blog.csdn.net/qq_39880374/article/details/103427810>
<https://club.rt-thread.org/ask/article/a082a4eabcfe44bb.html>
<https://blog.csdn.net/qq_43332314/article/details/128363320>
## SD Type

- SD (不大于2G)
- SDHC(大于 2G 不大于32G)
- SDXC(大于 32G，不大于 2TGB)

## 命令分类
- 广播命令
- 寻址（点对点）命令

## SD卡系统有3种模式多种状态

### 无效模式（Inactive Mode）

### 卡识别模式

在 卡识别模式下，主机会复位所有处于 “卡识别模式” 的卡，确认工资电压范围，识别卡
并且要求他们发布相对卡地址（Relatvie Card Address）
这个操作是通过各自的 CMD 线完成的。卡识别模式下，所有数据通信都只通过数据线完成

- Idle State
- Ready State
- Identification State

#### 卡的复位
当卡上电或者收到 GO_IDLE_STATE (CM0) 命令后，卡即可进入 Idle  State
这个模式的传输都是只通过 CMD 线来完成，SPI 无感
该复位是软复位，对于 Inactive 模式，无法复位该模式
此状态下，会将 RCA （相对地址）设置为 0
和一个默认的驱动级寄存器设置（最低速度，最高驱动电流能力）

#### 工作电压验证
每个卡的最高和最低工资电压都存在 OCR，只有当电压匹配时，CID 和 CSD 的数据才能真正传输给主机

####

#### 工作电压验证

## SD Card Registors

| name | bits | Description |
|-|-|-|
|CID| 128 Bit| 卡识别号|
|RCA| 16 | 卡的本地系统地址，初始化时，动态地由卡建议，主机核准|
| DSR | 16| 驱动级寄存器（Driver Stage Register）, 配置卡的输出驱动|
| CSD| 128| Card Specific Data, 卡的操作条件信息|
| SCR| 64| SD Configuration Register, SD 卡特殊特性信息 |
| OCR| 32| Operation conditions register |
| SSR| 512 | SD Status , SD 卡专有特征信息|
| CSR| 32| Card Status, 卡状态信息|

## Command Format
- 固定 64 bit
- bit0 起始位，始终为 0
- bit1 传输方向，1表示命令
- 剩下6bit，支持 64个指令
- 32bit 为参数/地址
- 倒数 7bit 为 CRC： SPI 通讯不管该位，默认关闭，只有 SDIO 才校验
- 倒数 1 bit 为停止位，始终为 0

## 专属 SPI 传输指令

## 相应
