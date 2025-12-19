typedef struct {

} bq2562x_reg_init_t;
// drv_bq2562x_reg_write(bq2562x_reg_init_t init_data,
// sizeof(bq2562x_reg_init_t))

typedef enum {
    BQ2562X_TIMER_RESET,
    BQ2562X_STATUS,   
}bq2562x_cmd_t;

typedef enum {
    BQ25620,
    BQ25622,
    BQ_UNKONWN;
}
