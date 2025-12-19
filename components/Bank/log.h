cntlr_region_info_t {
    flash_c
}

/**不是强关联得结构体*/
typedef struct event_log_context {
    machine_t sm;
    data_log_infor_t *event_info;
    cntlr_region_info_t ;
}event_log_t

/**强关联的，不会有太多的级联*/
typedef struct data_log_info{

}data_log_t;