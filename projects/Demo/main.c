/**
 * @file main.c
 * @brief XinYi Framework Comprehensive Demo
 * @version 1.0.0
 * @date 2026-02-28
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"
#include "xy_os.h"
#include "xy_device.h"
#include "xy_eeprom_24xx.h"
#include "xy_oled_ssd1306.h"
#include "xy_mpu6050.h"
#include "xy_bmp280.h"
#include "xy_sht30.h"

/* ==================== Global Variables ==================== */

static xy_eeprom_24xx_t eeprom;
static xy_oled_ssd1306_t oled;
static xy_mpu6050_t mpu;
static xy_bmp280_t bmp;
static xy_sht30_t sht;

static uint8_t sensor_task_stack[512];
static xy_os_thread_t sensor_task_thread;

static uint8_t display_task_stack[512];
static xy_os_thread_t display_task_thread;

/* ==================== Sensor Task ==================== */

static void sensor_task(void *arg)
{
    (void)arg;
    
    xy_log_i("Sensor task started\n");
    
    /* Initialize sensors */
    xy_mpu6050_init(&mpu, I2C1);
    xy_bmp280_init(&bmp, I2C1);
    xy_sht30_init(&sht, I2C1);
    
    while (1) {
        /* Read MPU6050 */
        if (xy_mpu6050_read(&mpu) == XY_DEVICE_OK) {
            xy_log_d("MPU6050 - Accel: X=%d, Y=%d, Z=%d\n", 
                     mpu.accel_x, mpu.accel_y, mpu.accel_z);
        }
        
        /* Read BMP280 */
        if (xy_bmp280_read(&bmp) == XY_DEVICE_OK) {
            xy_log_d("BMP280 - Temp=%d.%02d°C, Pressure=%luPa\n", 
                     bmp.temperature / 100, bmp.temperature % 100,
                     bmp.pressure);
        }
        
        /* Read SHT30 */
        if (xy_sht30_read(&sht) == XY_DEVICE_OK) {
            xy_log_d("SHT30 - Temp=%d.%02d°C, Humidity=%d.%02d%%RH\n", 
                     sht.temperature / 100, sht.temperature % 100,
                     sht.humidity / 100, sht.humidity % 100);
        }
        
        /* Save to EEPROM */
        uint8_t data[4];
        data[0] = (mpu.accel_x >> 8) & 0xFF;
        data[1] = mpu.accel_x & 0xFF;
        data[2] = (bmp.temperature >> 8) & 0xFF;
        data[3] = bmp.temperature & 0xFF;
        xy_eeprom_24xx_write(&eeprom, 0, data, 4);
        
        xy_os_delay(1000);
    }
}

/* ==================== Display Task ==================== */

static void display_task(void *arg)
{
    (void)arg;
    
    xy_log_i("Display task started\n");
    
    /* Initialize OLED */
    xy_oled_ssd1306_init(&oled, I2C1, 128, 64);
    xy_oled_ssd1306_clear(&oled);
    xy_oled_ssd1306_refresh(&oled);
    
    uint32_t tick = 0;
    
    while (1) {
        xy_oled_ssd1306_clear(&oled);
        
        /* Draw status */
        xy_oled_ssd1306_draw_string(&oled, 0, 0, "XinYi Demo", true);
        
        /* Draw tick counter */
        char buf[32];
        snprintf(buf, sizeof(buf), "Tick: %lu", tick);
        xy_oled_ssd1306_draw_string(&oled, 0, 16, buf, true);
        
        /* Draw sensor data */
        snprintf(buf, sizeof(buf), "T: %d.%02dC", 
                 bmp.temperature / 100, bmp.temperature % 100);
        xy_oled_ssd1306_draw_string(&oled, 0, 32, buf, true);
        
        snprintf(buf, sizeof(buf), "H: %d%%", sht.humidity / 100);
        xy_oled_ssd1306_draw_string(&oled, 0, 48, buf, true);
        
        xy_oled_ssd1306_refresh(&oled);
        
        tick++;
        xy_os_delay(1000);
    }
}

/* ==================== Main ==================== */

int main(void)
{
    xy_log_i("XinYi Framework Demo Starting...\n");
    
    /* Initialize OS */
    xy_os_kernel_init();
    
    /* Initialize EEPROM */
    xy_eeprom_24xx_init(&eeprom, I2C1, 0x50, 64, 32768);
    
    /* Create sensor task */
    xy_os_thread_create(
        &sensor_task_thread,
        "Sensor",
        sensor_task,
        NULL,
        5,
        sensor_task_stack,
        sizeof(sensor_task_stack)
    );
    
    /* Create display task */
    xy_os_thread_create(
        &display_task_thread,
        "Display",
        display_task,
        NULL,
        5,
        display_task_stack,
        sizeof(display_task_stack)
    );
    
    xy_log_i("Demo started!\n");
    
    /* Start OS */
    xy_os_kernel_start();
    
    return 0;
}
