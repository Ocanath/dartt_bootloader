/*
 * init.h
 *
 *  Created on: Apr 11, 2026
 *      Author: redux
 */

#ifndef INC_INIT_H_
#define INC_INIT_H_
#include "main.h"
#include "m_dma_uart.h"

extern FDCAN_HandleTypeDef hfdcan1;

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_FDCAN1_Init(void);
void MX_USART2_UART_Init(void);


#endif /* INC_INIT_H_ */
