# Pandora STM32L475VE 板级验证与接线

本目录保存 Pandora STM32L475VE 的板级验证镜像、板级适配及 UART 捕获校验工具。
硬件参考为 Pandora V2.4；[原理图及来源](../../docs/hardware/pandora_stm32l475/README.md)
位于 `docs/hardware/pandora_stm32l475/`。

以下内容于 2026-11-21 根据当前源码和已有实板记录整理，本次未重新上板测试。
“已验证”仅对应所引用记录中的器件、固件、接线与测试范围，不代表全部功能、精度或长期稳定性均通过。
所有 I²C 地址均为 **7 位地址**。

## 外接 I²C 设备：I2C2

已有实板证据的外接 I²C 器件共 **8 种**，共用以下连接：

| 信号 | MCU 引脚 | pinmux / 配置 |
|---|---|---|
| SCL | **PB10** | I2C2_SCL，AF4，开漏 |
| SDA | **PB11** | I2C2_SDA，AF4，开漏 |
| GND | GND | 所有模块与主板共地 |

验证固件使用 **100 kHz**。各模块的 SCL 并联到 PB10、SDA 并联到 PB11。
接线时使用 3.3 V 逻辑，上拉接至 3.3 V；模块供电脚需按实际模块规格确认，
不要将模块的 VIN 耐压视为 SDA/SCL 可接受的电平。下表记录验证时使用的地址，
不同地址选择脚配置的模块需先核对地址。

| 外接器件 | 已验证地址 | 额外接线 | 已有实板验证范围 |
|---|---|---|---|
| QMA6100P 加速度计 | `0x12` | INT1 → **PC6**；INT2 → **PD15** | `CHIP_ID=0x90`、静态连续采样；低有效、下降沿，两路真实 GPIO → EXTI callback 已验证 |
| SC7A22H 加速度计 | `0x18` | DRDY → **PB8 / EXTI8** | `WHO_AM_I=0x18`、配置回读、静态加速度、手动旋转响应、DRDY 电平与边沿计数 |
| HMC5883L 磁力计 | `0x1E` | DRDY → **PA4** | `"H43"` 身份、增益回读、连续磁场采样；观察到 PA4 高电平，未据此授予 EXTI 边沿证据 |
| BH1750 / GY-30 光照 | `0x23` | 无已确认的额外信号线 | 连续照度采样 |
| AHT30 温湿度 | `0x38` | 无已确认的额外信号线 | 温湿度采样及 CRC 校验 |
| MPU6050 / GY-521 六轴 | `0x68` | 中断接线未明确归属 | 连续加速度、陀螺仪原始采样 |
| L3G4200D 陀螺仪 | `0x69` | 中断接线未明确归属 | `WHO_AM_I=0xD3`、连续角速度采样 |
| BME680 环境传感器 | `0x77` | 无已确认的额外信号线 | `CHIP_ID=0x61`、补偿后的压力、气体等采样 |

验证入口和证据：

- [i2c2_sensors_main.c](i2c2_sensors_main.c)：除 QMA6100P 外的七器件组合验证镜像。
  当前入口会初始化全部七器件，缺少任一器件可能在初始化或读数失败后停止。
- [I2C2 七器件验证记录](../../docs/validation/i2c2-sensor-evidence.md)。
- [qma6100p_main.c](qma6100p_main.c)：QMA6100P 独立镜像，探测 `0x12/0x13`，
  实板捕获确认的是 `0x12`。
- [QMA6100P 真实 GPIO 边沿证据](../../docs/validation/evidence/pandora-stm32l475/2026-09-27/qma6100p-142e515c.json)：
  1 秒窗口内 PC6/PD15 分别记录 262/263 次边沿；
  [原始 UART](../../docs/validation/evidence/pandora-stm32l475/2026-09-27/qma6100p-142e515c-uart.txt)。

`i2c2_sensors_main.c` 还将 **PB9、PC4** 配置为输入并打印电平，
但现有记录未明确它们分别接哪个模块，不能据此指定为 MPU6050 或 L3G4200D 的中断线。
上述记录不构成八器件同时运行的验证；QMA6100P 使用独立镜像。

## 板载 I²C 器件

板载已有实板验证的 I²C 器件共 **3 种**，无需外接跳线。

| 板载器件 | 地址 | 当前独立验证镜像的路径 | SCL | SDA | 已有实板验证范围 |
|---|---|---|---|---|---|
| AHT10 温湿度 | `0x38` | 软件 I2C4，GPIO 开漏 | **PD6** | **PC1** | 初始化、ACK、连续温湿度采样 |
| AP3216C 光照 / 接近 / 红外 | `0x1E` | 硬件 I2C3，AF4 | **PC0** | **PC1** | 连续采样、既有光学刺激记录、访问空地址 NACK 后恢复访问器件 |
| ICM20608 六轴 | `0x68` | 软件 I2C3，GPIO 开漏 | **PC0** | **PC1** | `WHO_AM_I=0xAE`、配置回读、静态连续采样 |

这里的“软件 I2C4”是板级软件总线名称，不表示启用了 MCU 硬件 I2C4 外设。
板载两组连接**共用 PC1 数据线**；AP3216C 与 ICM20608 的当前独立镜像分别使用硬件和软件驱动。
已有独立镜像记录不代表三个板载器件并发访问已验证；集成时需统一引脚控制和总线访问。

板载 AHT10 / AP3216C / ICM20608 分别与外接 AHT30 / HMC5883L / MPU6050 地址相同，
当前接线通过不同总线区分。不要直接将这些同地址器件迁移到同一总线。

源码及证据：

- [sensor_aht10_main.c](sensor_aht10_main.c)、[sensor_ap3216c_main.c](sensor_ap3216c_main.c)、
  [sensor_icm20608_main.c](sensor_icm20608_main.c)。
- [软件 I²C 引脚定义](pandora_soft_i2c.c)、[硬件 I2C3 引脚配置](pandora_hw_i2c.c)。
- [组件证据台账](../../docs/validation/component-evidence-matrix.md)：
  2026-10-28 AHT10 canonical owner 实板 B1、2026-10-29 AP3216C / ICM20608 canonical owners 实板 B1。
- [较早的板载三传感器验证记录](../../docs/validation/xinyi-pandora-onboard-sensors-refresh-2026-09-09.md)。

## 其他器件及 pinmux

| 器件 / 接口 | 连接与 pinmux | 已有实板验证范围 |
|---|---|---|
| NRF24L01 无线接口 / 模块 | SPI2 AF5：**PB13=SCK、PB14=MISO、PB15=MOSI**；**PD5=CSN、PD4=CE、PD3=IRQ** | 寄存器访问、一次带 ACK 发送；PRX 窗口超时，未观察到接收 payload，未授予 IRQ 跳变证据 |
| 插入的 TF 卡 | SPI1 AF5：**PA5=SCK、PA6=MISO、PA7=MOSI**；**PC3=CS** | SDHC 初始化、只读 block 0；未验证写入、文件系统或热插拔 |
| 板载 W25Q128 | QSPI AF10：**PE10=CLK、PE11=NCS、PE12/PE13/PE14/PE15=IO0/IO1/IO2/IO3** | 身份、擦写读回、四线访问及受限恢复；具体事务和恢复条件以证据台账为准 |
| 板载 ST7789 屏 | SPI3 AF6：**PB3=SCK、PB5=MOSI**；**PD7=CS、PB4=D/C、PB6=RESET、PB7=背光** | 固定初始方向的颜色 / 图案视觉 B1；旋转有命令链证据，四方向视觉确认仍待完成；当前背光用 GPIO 控制 |
| 板载 RGB LED | **PE7=红、PE8=绿、PE9=蓝**，低有效 | 控制序列、最终关断状态 |
| 板载蜂鸣器 | **PB2**，高有效 | 发声控制序列、最终关闭 |
| 板载电机驱动 | **PA1=MOTOR_A/IA、PA0=MOTOR_B/IB** | 固定正转短短长振动、最终 L/L 待机停止；未验证反转、制动或 PWM |
| KEY0 | **PD10**，低有效 | 实际按键事件；PD8/PD9 虽在 smoke 中配置为输入，不据此声明另两键已验证 |
| 调试串口 USART1 | AF7：**PA9=TX、PA10=RX**，**115200 8N1** | 实板日志捕获；外接 USB-TTL 时 PA9 → 适配器 RX、PA10 → 适配器 TX，并共地 |

NRF24 在现有证据台账中按 Pandora onboard 接口记录，不能仅凭验证入口判定为外接跳线设备。
明确的外接 I²C 清单见上文；TF 卡是插入板载卡座的介质。

对应源码为 [nrf24l01_main.c](nrf24l01_main.c)、[tf_card_main.c](tf_card_main.c)、
[rtos_main.c](rtos_main.c)、[st7789_main.c](st7789_main.c)、[rgb_led_main.c](rgb_led_main.c)、
[buzzer_main.c](buzzer_main.c)、[motor_main.c](motor_main.c) 和 [main.c](main.c)。

验证依据：

- [组件证据台账](../../docs/validation/component-evidence-matrix.md)：Storage、Display、Actuator、NRF24 的当前证据边界。
- [TF 只读验证结果](../../docs/validation/evidence/pandora-stm32l475/2026-09-29/tf-read-1aa7e3e2.json)。
- [ST7789 验证记录](../../docs/validation/xinyi-pandora-st7789-bounded-visual-smoke-2026-09-06.md)。
- [板级 smoke 记录](../../docs/validation/xinyi-pandora-stm32l475-board-smoke-record.md)：KEY0、串口及部分存储恢复历史记录。

这些条目汇总多张独立验证镜像的结果，不代表一张固件同时启用了全部器件。
