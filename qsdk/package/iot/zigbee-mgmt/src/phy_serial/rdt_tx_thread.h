#ifndef ZB_SERIAL_TX_THREAD_H
#define ZB_SERIAL_TX_THREAD_H

#include "common_def.h"

int init_rdt_tx(void);

int rdt_send(serial_tx_buf_t *pbuf);

int is_ack_to_last_data(zb_uint8 seq);

//called by rx thread. notify tx thread to free buffer.
void rdt_tx_data_acked(void);

#endif

