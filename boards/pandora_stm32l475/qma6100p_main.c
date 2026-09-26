#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_qma6100p.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT required"
#endif

static UART_HandleTypeDef uart1;
static I2C_HandleTypeDef i2c2;
static xy_qma6100p_t qma;
static volatile uint32_t int1_edges;
static volatile uint32_t int2_edges;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }
void EXTI9_5_IRQHandler(void) { xy_hal_gpio_irq_handler(GPIOC, 6U); }
void EXTI15_10_IRQHandler(void) { xy_hal_gpio_irq_handler(GPIOD, 15U); }
static void edge(void *arg) { (*(volatile uint32_t *)arg)++; }
static void stop(void) { __disable_irq(); for (;;) {} }
static void text(const char *s){size_t n=0;while(s[n])++n;(void)xy_hal_uart_send(&uart1,(const uint8_t*)s,n,100U);}
static void num(int32_t v){char b[16];size_t p=sizeof(b);uint32_t x=v<0?(uint32_t)(-v):(uint32_t)v;b[--p]=0;do{b[--p]=(char)('0'+x%10U);x/=10U;}while(x);if(v<0)b[--p]='-';text(&b[p]);}
static void hex(uint8_t v){static const char h[]="0123456789ABCDEF";char b[2]={h[v>>4],h[v&15U]};(void)xy_hal_uart_send(&uart1,(uint8_t*)b,2U,100U);}
static void fail(const char *s, xy_error_t r){text(s);text(" result=");num(r);text("\r\n");stop();}

static void platform_init(void)
{
    const xy_hal_gpio_config_t uart_pins={XY_HAL_GPIO_MODE_AF,XY_HAL_GPIO_PULL_UP,XY_HAL_GPIO_OTYPE_PP,XY_HAL_GPIO_SPEED_VERY_HIGH,GPIO_AF7_USART1};
    const xy_hal_gpio_config_t i2c_pins={XY_HAL_GPIO_MODE_AF,XY_HAL_GPIO_PULL_UP,XY_HAL_GPIO_OTYPE_OD,XY_HAL_GPIO_SPEED_VERY_HIGH,GPIO_AF4_I2C2};
    const xy_hal_uart_config_t uart_cfg={115200U,XY_HAL_UART_WORDLEN_8B,XY_HAL_UART_STOPBITS_1,XY_HAL_UART_PARITY_NONE,XY_HAL_UART_FLOWCTRL_NONE,XY_HAL_UART_MODE_TX_RX};
    const xy_hal_i2c_config_t i2c_cfg={100000U,XY_HAL_I2C_ADDR_7BIT,XY_HAL_I2C_DUTY_2,0U,0U};
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();__HAL_RCC_USART1_CLK_ENABLE();__HAL_RCC_I2C2_CLK_ENABLE();
    if(xy_hal_gpio_init(GPIOA,9U,&uart_pins)!=XY_HAL_OK||xy_hal_gpio_init(GPIOA,10U,&uart_pins)!=XY_HAL_OK)stop();
    uart1.Instance=USART1;if(xy_hal_uart_init(&uart1,&uart_cfg)!=XY_HAL_OK)stop();
    if(xy_hal_gpio_init(GPIOB,10U,&i2c_pins)!=XY_HAL_OK||xy_hal_gpio_init(GPIOB,11U,&i2c_pins)!=XY_HAL_OK)stop();
    i2c2.Instance=I2C2;i2c2.Init.Timing=0x10909CECU;if(xy_hal_i2c_init(&i2c2,&i2c_cfg)!=XY_HAL_OK)stop();
    if(xy_hal_gpio_attach_irq(GPIOC,6U,XY_HAL_GPIO_IRQ_RISING,edge,(void*)&int1_edges)!=XY_HAL_OK)stop();
    if(xy_hal_gpio_attach_irq(GPIOD,15U,XY_HAL_GPIO_IRQ_RISING,edge,(void*)&int2_edges)!=XY_HAL_OK)stop();
}

int main(void)
{
    uint8_t address=0U;
    uint8_t id=0U;
    xy_error_t result;
    xy_qma6100p_interrupt_config_t irq_config;
    if(xy_hal_sys_init()!=XY_HAL_OK||pandora_platform_startup()!=0)stop();
    platform_init();
    text("PANDORA QMA6100P I2C2 PROBE\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    if(xy_hal_i2c_is_device_ready(&i2c2,XY_QMA6100P_ADDR_LOW,2U,20U)==XY_HAL_OK)address=XY_QMA6100P_ADDR_LOW;
    else if(xy_hal_i2c_is_device_ready(&i2c2,XY_QMA6100P_ADDR_HIGH,2U,20U)==XY_HAL_OK)address=XY_QMA6100P_ADDR_HIGH;
    else fail("QMA6100P_NOT_FOUND",XY_DEVICE_NOT_FOUND);
    if(xy_hal_i2c_mem_read(&i2c2,address,XY_QMA6100P_REG_CHIP_ID,&id,1U,100U)!=XY_HAL_OK)fail("QMA6100P_ID_IO_ERROR",XY_DEVICE_IO_ERROR);
    text("QMA6100P_ADDR=0x");hex(address);text(" CHIP_ID=0x");hex(id);text("\r\n");
    result=xy_qma6100p_init(&qma,&i2c2,address);if(result!=XY_DEVICE_OK)fail("QMA6100P_INIT_ERROR",result);
    result=xy_qma6100p_configure_data_ready_interrupts(&qma,1U,1U);if(result!=XY_DEVICE_OK)fail("QMA6100P_IRQ_CONFIG_ERROR",result);
    result=xy_qma6100p_read_interrupt_config(&qma,&irq_config);if(result!=XY_DEVICE_OK)fail("QMA6100P_IRQ_READBACK_ERROR",result);
    text("QMA6100P_IRQ_CONFIG en=0x");hex(irq_config.enable1);text(" int1_map=0x");hex(irq_config.map_int1);
    text(" int2_map=0x");hex(irq_config.map_int2);text(" pin=0x");hex(irq_config.pin_config);
    text(" cfg=0x");hex(irq_config.interrupt_config);text("\r\n");
    text("QMA6100P_IRQ_MAP INT1=PC6 INT2=PD15 ACTIVE=HIGH\r\n");
    for(uint32_t n=0;n<40U;n++){
        xy_qma6100p_raw_t raw;xy_qma6100p_accel_t a;uint8_t status=0U;
        result=xy_qma6100p_read_interrupt_status(&qma,&status);if(result!=XY_DEVICE_OK)fail("QMA6100P_STATUS_ERROR",result);
        result=xy_qma6100p_read_raw(&qma,&raw);if(result!=XY_DEVICE_OK)fail("QMA6100P_RAW_ERROR",result);
        result=xy_qma6100p_read_accel(&qma,&a);if(result!=XY_DEVICE_OK)fail("QMA6100P_ACCEL_ERROR",result);
        text("QMA6100P_SAMPLE n=");num((int32_t)n);text(" raw=");num(raw.x);text(",");num(raw.y);text(",");num(raw.z);
        text(" mg=");num(a.x_mg);text(",");num(a.y_mg);text(",");num(a.z_mg);text(" status=0x");hex(status);
        text(" int1_level=");num(xy_hal_gpio_read(GPIOC,6U));text(" int2_level=");num(xy_hal_gpio_read(GPIOD,15U));
        text(" int1_edges=");num((int32_t)int1_edges);text(" int2_edges=");num((int32_t)int2_edges);text("\r\n");
        xy_hal_delay_ms(25U);
    }
    text("QMA6100P_PROBE_DONE\r\n");
    for(;;)xy_hal_delay_ms(1000U);
}
