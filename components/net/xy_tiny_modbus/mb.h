typedef enum {
    MB_MODE_RTU,
    MB_MODE_ASCII,
    MB_MODE_TCP
}MB_TRANS_MODE;

/*
struct _modbus {
    int slave;

    int s;
    int debug;
    response_timeout
    byte_timeout
    indication_timeout


}
*/

/**
size_t mb_set_slave(mb_t *ctx, int slave)
size_t mb_get_slave(mb_t *ctx)
size_t mb_set_socket(mb_t *ctx, int s)
size_t mb_get_socket(mb_t *ctx)

size_t mb_get_response_timeout(mb_t *ctx, uint32_t *to_sec, uint32_t *to_usec)
size_t mb
*/






