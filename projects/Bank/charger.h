struct chrg_context {
    tx_buffer, rx_buffer, ic_info, uint32_t energy_accumulator, uint8_t status;
    uint8_t i2c_error_retires;
    /** flag indicating periodic monitoring in progress*/
    uint8_t i2c_periodic_monitoring;
    /** flag indicating periodic usb detection monitoring in progress*/
    uint8_t usb_periodic_monitoring;
    /** Mininum system voltage limit SYS_MIN in BQ/SC unit*/
    uint8_t v_sys_min;
    /**Precharge current limit in BQ/SC unit*/
    uint8_t precharge_current;

    /** Termination current limit in BQ/SC unit*/
    uint8_t term_current;
};


enum charge_state {
    chrg_state_enableing,
    chrg_state_disableing,
    chrg_state_configuring, // set_chrg_state(xxx)
    chrg_state_reseting,
    chrg_state_ilde,
    chrg_state_initializing,
    chrg_state_initialized,
    chrg_state_charging,
    chrg_state_reset,
    chrg_state_detecting,
    chrg_state_detecting,
    chrg_state_detecting_non_standard
};
/**
 * chrg_init(void)
 * chrg_reset(void)
 * chrg enable(void)
 * chrg_disable(void)
 * chrg_configure(void)
 * chrg_enable_measurement(void)
 * chrg_disable_measurement(void)
 * chrg_low_power(void)
 * chrg_shipping_mode(void)
 * chrg_exit_shipping_mode(void)
 */
void chrg_running(void);

/** enable i2c monitorng*/
void chrg_enable_monitoring(void)

    void chrg_disable_monitoring(void);

void chrg_update_target_volt_current(void);

void chrg_start_usb_detection(void);

void chrg_stop_usb_detection(void);

usb_charger_type_t chrg_usb_detection_type(void);

void chrg_usb_detection_non_standard_type(void);

void chrg_process(void);
