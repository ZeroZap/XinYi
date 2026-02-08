typedef struct {
    uint8_t soc_idle;
    uint8_t soc_charging;
    int8_t temp; // if tem != bank , update temp ...
} batt_ctx_t;