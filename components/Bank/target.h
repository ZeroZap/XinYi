#define SUPPORT_CHARGER_BQ2562X
#define CONFIG_CHARGER_BQ252620
#define BQ2562X_I2C_SLAVE_ADDRESS 0xD6
// USB 5V INPUT DET
#define VBUS_DET_PORT GPIOA
#define VBUS_DET_PIN  GPIO_PIN_2
@-156, 34 + 161,
    68 @ @
#define USB_CC2_ADC_GPIO_MODE hal_gpiomode_ain
#define USB_CC2_ADC_GPIO_PULL hal_gpio_pin_pull_none

#define CHARGE_DRV_I2C_PERIPHERAL 3
#define CHARGE_DRV_I2C_TRANSMODE  i2c_polling_mode
#define CHARGE_DRV_I2C_ADDMODE    i2c_address_7bit
#define CHARGE_DRV_I2C_ADDERSS    BQ2562X_I2C_SLAVE_ADDRESS
#define CHARGE_DRV_I2C_FREQ       100
#define CHARGE_DRV_I2C_ROLEMODE   hal_i2c_master_mode
// BAT BANK CE CONTROL (BNK=POWER BANK)
#define BNK_CE_PORT      GPIOB
#define BNK_CE_PIN       GPIO_PIN_4
#define BNK_CE_GPIO_MODE hal_gpiomode_out_pp
#define BNK_CE_GPIO_PULL hal_gpio_pin_pull_up
#define BNK_CE_SET_VALUE GPIO_PIN_RESET

#define CHARGE_DRV_SCL_PORT      GPIOD
#define CHARGE_DRV_SCL_PIN       GPIO_PIN_12
#define CHARGE_DRV_SCL_GPIO_AF   hal_gpio_af4
#define CHARGE_DRV_SCL_GPIO_MODE hal_gpiomode_af_od
#define CHARGE_DRV_SCL_GPIO_PULL hal_gpio_pin_pull_none
#define CHARGE_DRV_SDA_PORT      GPIOD
#define CHARGE_DRV_SDA_PIN       GPIO_PIN_13
#define CHARGE_DRV_SDA_GPIO_AF   hal_gpio_af4
#define CHARGE_DRV_SDA_GPIO_MODE hal_gpiomode_af_od
#define CHARGE_DRV_SDA_GPIO_PULL hal_gpio_pin_pull_none
// BAT CHARGE STATUS
#define STAT_CHARGER_PORT      GPIOB
#define STAT_CHARGER_PIN       GPIO_PIN_7
#define STAT_CHARGER_GPIO_MODE hal_gpiomode_ipu
#define STAT_CHARGER_GPIO_PULL \
    hal_gpio_pin_pull_up // must be pull up  Otherwise it will consume power


#define CHARGE_DRV_CE_PORT        GPIOB
#define CHARGE_DRV_CE_PIN         GPIO_PIN_4
#define CHARGE_DRV_CE_GPIO_MODE   hal_gpiomode_out_pp
#define CHARGE_DRV_CE_GPIO_PULL   hal_gpio_pin_pull_none
#define CHARGE_DRV_CE_GPIO_SETVAL gpio_value_reset


#define CHARGE_DRV_STAT_PORT             GPIOB
#define CHARGE_DRV_STAT_PIN              GPIO_PIN_7
#define CHARGE_DRV_STAT_GPIO_MODE        hal_gpiomode_it_falling
#define CHARGE_DRV_STAT_GPIO_PULL        hal_gpio_pin_pull_up
#define CHARGE_DRV_STAT_IRQ_PRE_PRIORITY 5
// CHARGE IC INTERRUPT
#define BNK_INT_PORT      GPIOC
#define BNK_INT_PIN       GPIO_PIN_6
#define BNK_INT_GPIO_MODE hal_gpiomode_ipu
#define BNK_INT_GPIO_PULL hal_gpio_pin_pull_up

#define CHARGE_DRV_INT_PORT             GPIOC
#define CHARGE_DRV_INT_PIN              GPIO_PIN_6
#define CHARGE_DRV_INT_GPIO_MODE        hal_gpiomode_it_rising_falling
#define CHARGE_DRV_INT_GPIO_PULL        hal_gpio_pin_pull_up
#define CHARGE_DRV_INT_IRQ_PRE_PRIORITY 5
// CHARGE IC INPUT VOLTAGET DET OK
#define CHARGE_PG_PORT      GPIOC
#define CHARGE_PG_PIN       GPIO_PIN_9
#define CHARGE_PG_GPIO_MODE hal_gpiomode_ipu
#define CHARGE_PG_PULL      hal_gpio_pin_pull_none


#define CHARGE_DET_PORT             GPIOC
#define CHARGE_DET_PIN              GPIO_PIN_9
#define CHARGE_DET_GPIO_MODE        hal_gpiomode_it_rising_falling
#define CHARGE_DET_GPIO_PULL        hal_gpio_pin_pull_up
#define CHARGE_DET_IRQ_PRE_PRIORITY 5

#define CHARGE_EXT_PWR_DET_PORT             NULL
#define CHARGE_EXT_PWR_DET_PIN              GPIO_PIN_9
#define CHARGE_EXT_PWR_DET_GPIO_MODE        hal_gpiomode_it_rising_falling
#define CHARGE_EXT_PWR_DET_GPIO_PULL        hal_gpio_pin_pull_up
#define CHARGE_EXT_PWR_DET_IRQ_PRE_PRIORITY 5

#define CHARGE_DRV_CHIP_NAME           "bq25620"
#define CHARGE_DRV_CHIP_MAXVOL         4800
#define CHARGE_DRV_CHIP_MINVOL         3500
#define CHARGE_DRV_CHIP_VOLSTEP        10
#define CHARGE_DRV_CHIP_MAXCURR        3520
#define CHARGE_DRV_CHIP_MINCURR        80
#define CHARGE_DRV_CHIP_CURRSTEP       80
#define CHARGE_DRV_CHIP_INPUT_MAXVOL   16800
#define CHARGE_DRV_CHIP_INPUT_MINVOL   3800
#define CHARGE_DRV_CHIP_INPUT_VOLSTEP  80
#define CHARGE_DRV_CHIP_INPUT_MAXCURR  3200
#define CHARGE_DRV_CHIP_INPUT_MINCURR  100
#define CHARGE_DRV_CHIP_INPUT_CURRSTEP 20
    // ***************** BATT MODE  ********************** //