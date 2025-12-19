#ifndef _MB_PDU_H_
#define _MB_PDU_H_

#define MB_PDU_SIZE_MAX 253
#define MB_PDU_SIZE_MIN 1
#define MB_PDU_FUNC_OFFSET  0
#define MB_PDU_DATA_OFFSET  1

typedef void (*mb_frame_start)(void);
typedef void (*mb_frame_stop)(void);

typedef MB_EXCEPTION(*mb_frame_recv)(uint8_t *address, uint8_t **frame, uint16_t *length);
typedef MB_EXCEPTION(*mb_frame_write)(uint8_t *address, uint8_t **frame, uint16_t *length);

#endif