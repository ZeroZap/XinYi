#!/bin/bash

set -e  # 遇到错误立即退出

echo "=== XinYi 组件架构重组脚本 ==="
echo "开始时间: $(date)"

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 进入项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo -e "${YELLOW}步骤 1: 创建新目录结构${NC}"
mkdir -p components/drivers/sensor/{temperature,pressure,motion,adc}
mkdir -p components/drivers/display/{oled,lcd,led}
mkdir -p components/drivers/storage/{eeprom,flash,sdcard}
mkdir -p components/drivers/power/{charger,fuel_gauge}
mkdir -p components/drivers/wireless/rfid
mkdir -p components/drivers/system/{key,rtc,watchdog}
echo -e "${GREEN}✓ 目录结构已创建${NC}"

echo -e "${YELLOW}步骤 2: 迁移传感器驱动${NC}"
if [ -f "components/device/xy_sht30.c" ]; then
    mkdir -p components/drivers/sensor/temperature/sht30
    mv components/device/xy_sht30.* components/drivers/sensor/temperature/sht30/
    echo -e "${GREEN}✓ SHT30 已迁移${NC}"
else
    echo -e "${YELLOW}  SHT30 不存在，跳过${NC}"
fi

if [ -f "components/device/xy_bmp280.c" ]; then
    mkdir -p components/drivers/sensor/pressure/bmp280
    mv components/device/xy_bmp280.* components/drivers/sensor/pressure/bmp280/
    echo -e "${GREEN}✓ BMP280 已迁移${NC}"
else
    echo -e "${YELLOW}  BMP280 不存在，跳过${NC}"
fi

if [ -f "components/device/xy_mpu6050.c" ]; then
    mkdir -p components/drivers/sensor/motion/mpu6050
    mv components/device/xy_mpu6050.* components/drivers/sensor/motion/mpu6050/
    echo -e "${GREEN}✓ MPU6050 已迁移${NC}"
else
    echo -e "${YELLOW}  MPU6050 不存在，跳过${NC}"
fi

if [ -f "components/device/xy_ads1115.c" ]; then
    mkdir -p components/drivers/sensor/adc/ads1115
    mv components/device/xy_ads1115.* components/drivers/sensor/adc/ads1115/
    echo -e "${GREEN}✓ ADS1115 已迁移${NC}"
else
    echo -e "${YELLOW}  ADS1115 不存在，跳过${NC}"
fi

echo -e "${YELLOW}步骤 3: 迁移显示驱动${NC}"
if [ -f "components/device/xy_oled_ssd1306.c" ]; then
    mkdir -p components/drivers/display/oled/ssd1306
    mv components/device/xy_oled_ssd1306.* components/drivers/display/oled/ssd1306/
    echo -e "${GREEN}✓ SSD1306 OLED 已迁移${NC}"
else
    echo -e "${YELLOW}  SSD1306 OLED 不存在，跳过${NC}"
fi

echo -e "${YELLOW}步骤 4: 迁移存储驱动${NC}"
if [ -f "components/device/xy_eeprom_24xx.c" ]; then
    mkdir -p components/drivers/storage/eeprom/24xx
    mv components/device/xy_eeprom_24xx.* components/drivers/storage/eeprom/24xx/
    echo -e "${GREEN}✓ EEPROM 24xx 已迁移${NC}"
else
    echo -e "${YELLOW}  EEPROM 24xx 不存在，跳过${NC}"
fi

echo -e "${YELLOW}步骤 5: 迁移 driver/ 目录内容${NC}"
if [ -d "components/driver/charger" ] && [ "$(ls -A components/driver/charger 2>/dev/null)" ]; then
    cp -r components/driver/charger/* components/drivers/power/charger/ 2>/dev/null || true
    echo -e "${GREEN}✓ 充电器驱动已迁移${NC}"
else
    echo -e "${YELLOW}  充电器驱动不存在或为空，跳过${NC}"
fi

if [ -d "components/driver/rfid" ] && [ "$(ls -A components/driver/rfid 2>/dev/null)" ]; then
    cp -r components/driver/rfid/* components/drivers/wireless/rfid/ 2>/dev/null || true
    echo -e "${GREEN}✓ RFID 驱动已迁移${NC}"
else
    echo -e "${YELLOW}  RFID 驱动不存在或为空，跳过${NC}"
fi

if [ -d "components/driver/sensor" ] && [ "$(ls -A components/driver/sensor 2>/dev/null)" ]; then
    cp -r components/driver/sensor/* components/drivers/sensor/ 2>/dev/null || true
    echo -e "${GREEN}✓ Sensor 驱动已迁移${NC}"
else
    echo -e "${YELLOW}  Sensor 驱动不存在或为空，跳过${NC}"
fi

if [ -d "components/driver/storage" ] && [ "$(ls -A components/driver/storage 2>/dev/null)" ]; then
    cp -r components/driver/storage/* components/drivers/storage/ 2>/dev/null || true
    echo -e "${GREEN}✓ Storage 驱动已迁移${NC}"
else
    echo -e "${YELLOW}  Storage 驱动不存在或为空，跳过${NC}"
fi

echo -e "${YELLOW}步骤 6: 迁移 drivers/ 目录内容${NC}"
if [ -d "components/drivers/xy_key" ]; then
    mv components/drivers/xy_key components/drivers/system/key
    echo -e "${GREEN}✓ 按键驱动已迁移${NC}"
else
    echo -e "${YELLOW}  按键驱动不存在，跳过${NC}"
fi

if [ -d "components/drivers/xy_rtc" ]; then
    mv components/drivers/xy_rtc components/drivers/system/rtc
    echo -e "${GREEN}✓ RTC 驱动已迁移${NC}"
else
    echo -e "${YELLOW}  RTC 驱动不存在，跳过${NC}"
fi

if [ -d "components/drivers/xy_sys" ]; then
    cp -r components/drivers/xy_sys/* components/drivers/system/ 2>/dev/null || true
    echo -e "${GREEN}✓ 系统驱动已迁移${NC}"
else
    echo -e "${YELLOW}  系统驱动不存在，跳过${NC}"
fi

echo -e "${YELLOW}步骤 7: 备份旧目录（不删除）${NC}"
if [ -d "components/driver" ]; then
    mv components/driver components/driver.backup
    echo -e "${GREEN}✓ driver/ 已重命名为 driver.backup/${NC}"
fi

echo -e "${GREEN}=== 重组完成 ===${NC}"
echo "结束时间: $(date)"
echo ""
echo -e "${YELLOW}下一步:${NC}"
echo "1. 检查新目录结构: components/drivers/"
echo "2. 更新 CMakeLists.txt"
echo "3. 更新 Kconfig"
echo "4. 更新代码中的 #include 路径"
echo "5. 编译测试"
echo "6. 如果一切正常，删除备份: rm -rf components/driver.backup"
echo ""
echo -e "${RED}注意: 旧目录已重命名为 .backup，请验证后手动删除${NC}"
