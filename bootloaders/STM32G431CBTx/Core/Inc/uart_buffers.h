/*
 * uart_buffers.h
 *
 *  Created on: Jan 7, 2026
 *      Author: Ocanath Robotman
 */

#ifndef INC_UART_BUFFERS_H_
#define INC_UART_BUFFERS_H_
#include <stdint.h>

#define UART_BUF_SIZE 64

extern uint8_t gl_uart_tx_buf[UART_BUF_SIZE];
extern uint8_t gl_uart_rx_buf[UART_BUF_SIZE];
extern uint8_t gl_uart_rx_decoded_buf[UART_BUF_SIZE];


#endif /* INC_UART_BUFFERS_H_ */
