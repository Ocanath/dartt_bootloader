/*
 * dartt_bl_stubs.c
 *
 *  Created on: Apr 7, 2026
 *      Author: ocanath
 */
#include "dartt_bl_stubs.h"
#include "dartt_bl_linker.h"
#include "cobs.h"
#include "dartt.h"
#include "main.h"

dartt_bl_t gl_bootloader = {};
dartt_mem_t bootloader_alias = {
		.buf = (unsigned char*)(&gl_bootloader),
		.size = sizeof(gl_bootloader)
};


typedef void (*pFunction)(void); /*!< Function pointer definition */


uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_cleanup_system(void)
{
	__disable_irq();
//	HAL_DMA_DeInit(&hdma_usart2_rx);
//	HAL_DMA_DeInit(&hdma_usart2_tx);
//	HAL_UART_DeInit(&huart2);
    HAL_RCC_DeInit();
	HAL_DeInit();
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_start_application(dartt_bl_t * pbl)
{
    uint32_t JumpAddress = *(__IO uint32_t*)(dartt_bl_get_app_start() + 4);
    pFunction Jump       = (pFunction)JumpAddress;

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    SCB->VTOR = (__IO uint32_t)(dartt_bl_get_app_start());
    __set_MSP(*(__IO uint32_t*)dartt_bl_get_app_start());
    __enable_irq();
    Jump();
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size)
{

	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages)
{

	return DARTT_BL_SUCCESS;
}
