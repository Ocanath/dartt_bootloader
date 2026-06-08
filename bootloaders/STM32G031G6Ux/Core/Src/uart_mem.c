/*
 * uart_buffers.c
 *
 *  Created on: Jun 6, 2026
 *      Author: redux
 */
#include "uart_mem.h"
#include "init.h"

uint8_t gl_uart2_rx_buf[UART_BUF_SIZE] = {};
uint8_t gl_uart2_rx_decoded_buf[UART_BUF_SIZE] = {};
uint8_t gl_uart2_tx_buf[UART_BUF_SIZE] = {};

dma_uart_t m_huart2;

