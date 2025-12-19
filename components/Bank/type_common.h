enum i2c_cntlr_slave_state {
    i2c_cntrl_slave_state_free,
    slave_state_write_req,
    slave_state_write_inprogress,
    slave_state_write_done,
    slave_state_read_req,
    slave_state_read_inprogress,
    slave_state_read_done,
    slave_state_nack_error,
    slave_state_timeout_error,
    slave_state_arlo_error,
    slave_state_bus_error
};

enum flash_cntlr_state {
    flash_cntrl_state_free,
    write_req,
    write_in_progress,
    write_done,
    read_req,
    read_in_progress,
    read_done,
    erase_sector_req,
    erase_32k_req,
    erase_64k_req,
    erase_in_progress,
    erase_done
};

enum fw_uug_state {
    free,
    write_req,
    read_xx,
    erase_req,
    erase_in_progress,
    erase_done,
};

enum spi_cntlr_state{
    free, write_req, write_inprogres, write_done, read_req, ..read_done
};

enum platform_state{
    sleeping,
    off,
    running,
};

enum devcie_state {
    off_request,
    off_improgress,
    off,
    on_request,
    on,
    shipping_mode_request,
    power_down_request,
    power_down,
    secure_action
};

enum reset_ret {
    reset_req_none,
    reset_req_normal,
    reset_req_sbl
};

enum mt_comm_state {
    mt_commun_state_idle,
    mt_comm_state_active,
};

enum usb_dect_driver {
    usb_dect_driver_comp,
    usb_detect_driver_gpio,
};

enum usb_cxn_state {
    usb_cxn_state_disconnected,
    usb_cxn_state_detected,
    usb_cxn_state_connected,
};


enum usb_comm_state {
    usb_comm_state_not_initialized,
    usb_comm_state_initialized,
};

enum usb_host_current {
    usb_host_current_unknown,
    usb_host_current_no_current,
    usb_host_current_100ma,
    usb_host_current_500ma,
};

enum usb_charger_type
{
    none, sdp,cdp,dcp,dcp_hv,unknow,non1A, non2A, non2_1A,non_2_4a,
    // no cc? pd?
};

enum chrg_source {
    dcp_2050ma,
    sdp_500ma,
    sdp_1000ma,
    cdp_default_2050ma,
    cdp_audio_500ma,
    cdp_medium_1500ma,
    cdp_high_2050ma,
    sdp_scp_dock_2050ma,
    non_standard_1000ma,
    non_standard_2000ma,
    non_standard_2100ma,
    non_standard_2400ma,
    unknow_500ma,
    // others ???
};

enum chrg_chip {
    chrg_chip_firt = 0,
    chrg_chip_bq25xx = chrg_chip_first,
    bxx_other,
    chrg_chip_unkonw,
    chrg_chip_count = chrg_chip_unkown,
};

enum_led_driver_state {
    off, reseting, initializing, initialized
};

enum auth_state {
    failed = 0xAA,
    not_authenticated = 0x44,
    passed  = 0x55,
}

enum batt_level {
    unkown = 0,
    level_flat,
    level_critical,
    level_2_last_exp,
    level_low,
    level_medium,
    level_high,
    level_full,
};

enum data_log_status {
    ok, fail_read, fail_write, unknow
};

enum data_log_state {
    init, idle, multiple_read, write, erase_all, off
}