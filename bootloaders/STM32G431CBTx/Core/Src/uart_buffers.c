/*
 * uart_buffers.c
 *
 *  Created on: Jan 7, 2026
 *      Author: Ocanath Robotman
 */
#include "uart_buffers.h"


uint8_t gl_uart_tx_buf[UART_BUF_SIZE] = {};
uint8_t gl_uart_rx_buf[UART_BUF_SIZE] = {};
uint8_t gl_uart_rx_decoded_buf[UART_BUF_SIZE] = {};


