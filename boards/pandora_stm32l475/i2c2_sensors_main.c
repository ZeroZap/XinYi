#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_aht30.h"
#include "xy_bh1750.h"
#include "xy_bme680.h"
#include "xy_hmc5883l.h"
#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_l3g4200d.h"
#include "xy_mpu6050.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT required"
#endif

static UART_HandleTypeDef uart1;
static I2C_HandleTypeDef i2c2;
static xy_aht30_t aht30;
static xy_l3g4200d_t l3g;
static xy_bme680_t bme680;
static xy_bh1750_t bh1750;
static xy_mpu6050_t mpu6050;
static xy_hmc5883l_t hmc5883l;
void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }
static void stop(void) { __disable_irq(); for (;;) {} }
static void text(const char *s){size_t n=0;while(s[n])++n;(void)xy_hal_uart_send(&uart1,(const uint8_t*)s,n,100U);}
static void num(int32_t v){char b[16];size_t p=sizeof(b);uint32_t x=v<0?(uint32_t)(-v):(uint32_t)v;b[--p]=0;do{b[--p]=(char)('0'+x%10U);x/=10U;}while(x);if(v<0)b[--p]='-';text(&b[p]);}
static void hex(uint8_t v){static const char h[]="0123456789ABCDEF";char b[2]={h[v>>4],h[v&15U]};(void)xy_hal_uart_send(&uart1,(uint8_t*)b,2U,100U);}
static void fail(const char *s){text(s);text("\r\n");stop();}
static void platform_init(void){
 const xy_hal_gpio_config_t u={XY_HAL_GPIO_MODE_AF,XY_HAL_GPIO_PULL_UP,XY_HAL_GPIO_OTYPE_PP,XY_HAL_GPIO_SPEED_VERY_HIGH,GPIO_AF7_USART1};
 const xy_hal_gpio_config_t i={XY_HAL_GPIO_MODE_AF,XY_HAL_GPIO_PULL_UP,XY_HAL_GPIO_OTYPE_OD,XY_HAL_GPIO_SPEED_VERY_HIGH,GPIO_AF4_I2C2};
 const xy_hal_uart_config_t uc={115200U,XY_HAL_UART_WORDLEN_8B,XY_HAL_UART_STOPBITS_1,XY_HAL_UART_PARITY_NONE,XY_HAL_UART_FLOWCTRL_NONE,XY_HAL_UART_MODE_TX_RX};
 const xy_hal_i2c_config_t ic={100000U,XY_HAL_I2C_ADDR_7BIT,XY_HAL_I2C_DUTY_2,0U,0U};
 __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();__HAL_RCC_USART1_CLK_ENABLE();__HAL_RCC_I2C2_CLK_ENABLE();
 if(xy_hal_gpio_init(GPIOA,9U,&u)!=XY_HAL_OK||xy_hal_gpio_init(GPIOA,10U,&u)!=XY_HAL_OK)stop();
 uart1.Instance=USART1;if(xy_hal_uart_init(&uart1,&uc)!=XY_HAL_OK)stop();
 if(xy_hal_gpio_init(GPIOB,10U,&i)!=XY_HAL_OK||xy_hal_gpio_init(GPIOB,11U,&i)!=XY_HAL_OK)stop();
 i2c2.Instance=I2C2;i2c2.Init.Timing=0x10909CECU;if(xy_hal_i2c_init(&i2c2,&ic)!=XY_HAL_OK)stop();
 const xy_hal_gpio_config_t irq={XY_HAL_GPIO_MODE_INPUT,XY_HAL_GPIO_PULL_DOWN,XY_HAL_GPIO_OTYPE_PP,XY_HAL_GPIO_SPEED_LOW,0U};
 if(xy_hal_gpio_init(GPIOA,4U,&irq)!=XY_HAL_OK||xy_hal_gpio_init(GPIOB,8U,&irq)!=XY_HAL_OK||xy_hal_gpio_init(GPIOB,9U,&irq)!=XY_HAL_OK||xy_hal_gpio_init(GPIOC,4U,&irq)!=XY_HAL_OK)stop();
}
static void scan(void){uint32_t count=0;for(uint16_t a=8;a<=0x77;a++)if(xy_hal_i2c_is_device_ready(&i2c2,a,2U,10U)==XY_HAL_OK){text("I2C2_ACK=0x");hex((uint8_t)a);text("\r\n");count++;}text("I2C2_COUNT=");num((int32_t)count);text("\r\n");}
int main(void){
 if(xy_hal_sys_init()!=XY_HAL_OK||pandora_platform_startup()!=0) {
     stop();
 }
 platform_init();
 text("PANDORA I2C2 SENSOR PROBE\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");scan();
 uint8_t id=0;if(xy_hal_i2c_mem_read(&i2c2,0x77U,0xD0U,&id,1U,100U)!=XY_HAL_OK)fail("BME680_ID_IO_ERROR");text("BME680_CHIP_ID=0x");hex(id);text("\r\n");if(id!=0x61U)fail("BME680_ID_ERROR");

 if(xy_aht30_init(&aht30,&i2c2)!=XY_DEVICE_OK)fail("AHT30_INIT_ERROR");
 if(xy_l3g4200d_init(&l3g,&i2c2,0x69U)!=XY_DEVICE_OK)fail("L3G4200D_INIT_ERROR");
 if(xy_bme680_init(&bme680,&i2c2,0x77U)!=XY_DEVICE_OK)fail("BME680_INIT_ERROR");
 if(xy_bh1750_init(&bh1750,&i2c2,BH1750_ADDR_LOW)!=XY_BH1750_OK)fail("BH1750_INIT_ERROR");
 if(xy_mpu6050_init(&mpu6050,&i2c2)!=XY_MPU6050_OK)fail("MPU6050_INIT_ERROR");
 if(xy_hmc5883l_init(&hmc5883l,&i2c2)!=XY_DEVICE_OK)fail("HMC5883L_INIT_ERROR");
 if(xy_hmc5883l_set_gain(&hmc5883l,XY_HMC5883L_GAIN_8_10_GA)!=XY_DEVICE_OK)fail("HMC5883L_GAIN_ERROR");
 text("HMC5883L_GAIN=8.10GA\r\n");
 xy_hmc5883l_gain_t active_gain;
 if(xy_hmc5883l_get_gain(&hmc5883l,&active_gain)!=XY_DEVICE_OK)fail("HMC5883L_GAIN_READBACK_ERROR");
 text("HMC5883L_GAIN_READBACK=0x");hex((uint8_t)active_gain);text("\r\n");
 text("I2C2_SENSOR_IDENTITIES_OK\r\n");
 for(uint32_t n=0;n<20U;n++){xy_aht30_data_t a;xy_l3g4200d_data_t g;xy_bme680_data_t b;xy_mpu6050_raw_data_t m;xy_hmc5883l_data_t h={0};xy_hmc5883l_field_t f={0};float lux;uint8_t hready=0U;xy_error_t hr=XY_DEVICE_OK;xy_error_t fr=XY_DEVICE_OK;xy_error_t br=XY_DEVICE_BUSY;if(xy_aht30_read(&aht30,&a)!=XY_DEVICE_OK)fail("AHT30_READ_ERROR");if(xy_l3g4200d_read(&l3g,&g)!=XY_DEVICE_OK)fail("L3G4200D_READ_ERROR");for(uint32_t attempt=0U;attempt<5U&&br!=XY_DEVICE_OK;++attempt){br=xy_bme680_read(&bme680,&b);if(br!=XY_DEVICE_OK)xy_hal_delay_ms(20U);}if(br!=XY_DEVICE_OK){text("BME680_READ_ERROR result=");num(br);fail("");}if(xy_bh1750_read(&bh1750)!=XY_BH1750_OK||xy_bh1750_get_illuminance(&bh1750,&lux)!=XY_BH1750_OK)fail("BH1750_READ_ERROR");if(xy_mpu6050_read_raw(&mpu6050)!=XY_MPU6050_OK)fail("MPU6050_READ_ERROR");m=mpu6050.raw;if(xy_hmc5883l_data_ready(&hmc5883l,&hready)!=XY_DEVICE_OK)fail("HMC5883L_STATUS_ERROR");if(hready){hr=xy_hmc5883l_read(&hmc5883l,&h);if(hr==XY_DEVICE_OK)fr=xy_hmc5883l_read_field(&hmc5883l,&f);}text("SAMPLE aht_t_centi=");num(a.temperature_centi_c);text(" aht_rh_centi=");num((int32_t)a.humidity_centi_pct);text(" lux_x10=");num((int32_t)(lux*10.0F));text(" mpu_ax=");num(m.accel_x);text(" mpu_ay=");num(m.accel_y);text(" mpu_az=");num(m.accel_z);text(" mag_ready=");num(hready);text(" mag_result=");num(hr);text(" mag_field_result=");num(fr);text(" mag_x_mgauss=");num(f.x_mgauss);text(" mag_y_mgauss=");num(f.y_mgauss);text(" mag_z_mgauss=");num(f.z_mgauss);text(" gx_mdps=");num(g.x_mdps);text(" gy_mdps=");num(g.y_mdps);text(" gz_mdps=");num(g.z_mdps);text(" bme_pa=");num((int32_t)b.pressure_pa);text(" irq_pa4=");num(xy_hal_gpio_read(GPIOA,4U));text(" irq_pb8=");num(xy_hal_gpio_read(GPIOB,8U));text(" irq_pb9=");num(xy_hal_gpio_read(GPIOB,9U));text(" irq_pc4=");num(xy_hal_gpio_read(GPIOC,4U));text("\r\n");xy_hal_delay_ms(100U);}text("PANDORA_I2C2_SENSOR_PROBE_DONE\r\n");for(;;)xy_hal_delay_ms(1000U);
}
