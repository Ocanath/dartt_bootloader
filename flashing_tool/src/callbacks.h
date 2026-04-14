#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "cobs.h"
#include "dartt.h"
#include "serial.h"

#define _DARTT_SERIAL_BUFFER_SIZE 32
#define _DARTT_NUM_BYTES_COBS_OVERHEAD	2	//give cobs room to operate

#define DARTT_READ_CALLBACK_TIMEOUT -777

int _tx_blocking_callback(unsigned char addr, dartt_buffer_t * b, void * user_context, uint32_t timeout);
int _rx_blocking_callback(dartt_buffer_t * buf, void * user_context, uint32_t timeout);

#endif
