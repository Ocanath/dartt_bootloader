/*
 * m_uart.h
 *
 *  Created on: May 17, 2021
 *      Author: Ocanath
 */

#ifndef M_DMA_UART_H_
#define M_DMA_UART_H_
#include "main.h"
#include "stm32g4xx_it.h"
#include "cobs.h"
//#include "PPP.h"
#include "dartt.h"

/*The following structure is used to implement
 * the baremetal interrupt handler. The actual
 * handler should be populated with the handler function and a unique instance
 * of the uart_it_t structure.
 *
 *
 * */
typedef struct uart_it_t
{
	USART_TypeDef * Instance;
	DMA_Channel_TypeDef * rxdma;
	DMA_Channel_TypeDef * txdma;

	cobs_buf_t rx_mem;	//raw data buffer, encoded
	cobs_buf_t rx_decoded;	//cobs unstuffed - decoded buffer

	dartt_buffer_t rx_decode_alias;	//dartt buffer alias for decoded buffer
	payload_layer_msg_t rx_pld_msg;	//dartt helper structure


	cobs_buf_t tx_mem;	//raw data buffer - encoded
	dartt_buffer_t tx_buf_alias;	//dartt data buffer - unencoded. encoding invalidates this buffer due to referring to the same memory, so be sure to invalidate by setting this len=0 after an encode.
}uart_it_t;



extern uart_it_t m_huart2;

enum {ERROR_UART_BAD_INPUT = -1, SUCCESS_UART = 0};

//void m_uart_it_handler(uart_it_t * h);
void m_uart_it_handler(uart_it_t * h);
void m_uart_tx_start(uart_it_t * h, uint8_t * buf, int size);
void m_uart2_rx_cplt_callback(uart_it_t * h);
void m_uart_start_interrupts(uart_it_t * h);
void m_uart_rxdma_handler(DMA_HandleTypeDef *hdma);
void m_uart_txdma_handler(DMA_HandleTypeDef *hdma);
void m_uart_enable_rx_interrupt(uart_it_t * h);
void m_uart_disable_rx_interrupt(uart_it_t * h);
int m_uart_dma_transmit(uart_it_t * h);
uint32_t m_uart_bytes_to_send(uart_it_t * h);

#endif /* M_UART_H_ */
