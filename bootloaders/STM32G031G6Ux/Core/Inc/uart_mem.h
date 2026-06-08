/*
 * uart_mem.h
 *
 *  Created on: Jun 6, 2026
 *      Author: redux
 */

#ifndef INC_UART_MEM_H_
#define INC_UART_MEM_H_

#include "m_dma_uart.h"

#define UART_BUF_SIZE 128


extern uint8_t gl_uart2_tx_buf[UART_BUF_SIZE];
extern uint8_t gl_uart2_rx_buf[UART_BUF_SIZE];
extern uint8_t gl_uart2_rx_decoded_buf[UART_BUF_SIZE];


extern dma_uart_t m_huart2;


#endif /* INC_UART_MEM_H_ */
