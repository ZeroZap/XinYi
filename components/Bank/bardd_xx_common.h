typedef struct device_info {
    self_test_t self_test_status;
    platform_state_t platform_state;
    device_state_t state;
    // 复位类型
    reset_req_t reset;
    union {
        uint32_t faults_full
    };
    bist_error_data;
    uint8_t hw_version
    bool ship_mode;
}device_info_t;


/**
 * 
* spi, i2c, adc
*/


struct boad_chrg_info {
    chrg_state_t state;
    chrte_timer_t charge_timer;
    batt_level_t batt_level;
    chrge_source source;
    chrg_chip_t chip;
    uint16_t adc_batt_voltage;
    uint8_t faults;
    uint8_t input_current;
    uint16_t target_current;
    /**current measurement performed by bq chip*/
    uint8_t measured_current;
    uint8_t target_voltage;
    uint8_t precharge_current;
    uint8_t term_current;
    uint8_t system_voltage;
    int8_t level_percent;
    int8_t battery_temperature;
    int8_t ambient_temperature;
    uint8_t chrg_suspend;
    bool recharg_req;
};


struct board_usb_info {
    usb_detect_driver_t deriver;
    usb_cxn_state_t cxn_state;
    usb_chrg_type cxn_type;
    usb_comm_state comm_state;
    /** current USB IO event flag*/
    atomic_u16_t io_evt;
    usb_host_current_t host_current;
    usb_current_mode_t usb_curent_mode;
}



struct battery_soc{
uint16_t bat_0;
uint16_t bat_critical_low;	
uint16_t bat_critical_high;
uint16_t bat_25;
uint16_t bat_50;
uint16_t bat_75;
uint16_t bat_100;
}

// ctx 包含 操作
typedef struct xx_ctx {
	
}xx_ctx_t

// info 只有数据
typedef struct __info {
	
}__info_t