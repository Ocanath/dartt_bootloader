#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "cobs.h"
#include "dartt.h"
#include "serial.h"

int _tx_blocking_callback(unsigned char addr, dartt_buffer_t * b, void * user_context, uint32_t timeout);
int _rx_blocking_callback(dartt_buffer_t * buf, void * user_context, uint32_t timeout);

#endif
