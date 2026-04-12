/*
 * dartt_bl_stubs.c
 *
 *  Created on: Apr 7, 2026
 *      Author: ocanath
 */
#include "dartt_bl_stubs.h"
#include "dartt_bl_linker.h"
#include "init.h"
#include "cobs.h"
#include "dartt.h"
#include "m_dma_uart.h"

#define NUM_PAGES 		64
#define PAGE_SIZE 		0x800
#define WRITE_SIZE		8

dartt_bl_t gl_bootloader = {};
dartt_mem_t bootloader_alias = {
		.buf = (unsigned char*)(&gl_bootloader),
		.size = sizeof(gl_bootloader)
};


typedef void (*pFunction)(void); /*!< Function pointer definition */


uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
	pbl->attr.num_pages = NUM_PAGES;
	pbl->attr.page_size = PAGE_SIZE;
	pbl->attr.write_size = WRITE_SIZE;
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl)
{
	unsigned char dartt_misc_address = dartt_get_complementary_address((unsigned char)(pbl->fds.module_number & 0xFF));


//	if(HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) != 0)
//	{
//		HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &can_rx_header, can_rx_data.u8);
//		can_rx_alias.len = (can_rx_header.DataLength >> 16) & 0xF;
//		if (can_rx_header.Identifier == dartt_misc_address)
//		{
//		    int rc = dartt_frame_to_payload(&can_rx_alias, TYPE_ADDR_CRC_MESSAGE, PAYLOAD_ALIAS, &gl_can_rx_pld_msg);
//		    if(rc == DARTT_PROTOCOL_SUCCESS)
//		    {
//		    	dartt_parse_general_message(&gl_can_rx_pld_msg, TYPE_ADDR_CRC_MESSAGE, &bootloader_alias, &can_tx_alias);
//		    }
//		    if(can_tx_alias.len != 0 && rc == DARTT_PROTOCOL_SUCCESS)
//		    {
//		    	send_fdcan_frame(MASTER_MISC_ADDRESS, &can_tx_alias);
//		    }
//		}
//	}

	/*Handle DARTT over UART*/
	if(m_huart2.rx_decoded.length != 0)
	{
		if(m_huart2.rx_decoded.buf[0] == dartt_misc_address)
		{
			int rc = dartt_frame_to_payload(&m_huart2.rx_decode_alias, TYPE_SERIAL_MESSAGE, PAYLOAD_ALIAS, &m_huart2.rx_pld_msg);
			if(rc == DARTT_PROTOCOL_SUCCESS)
			{
				rc = dartt_parse_general_message(&m_huart2.rx_pld_msg, TYPE_SERIAL_MESSAGE, &bootloader_alias, &m_huart2.tx_buf_alias);
			}
			if(m_huart2.tx_buf_alias.len != 0 && rc == DARTT_PROTOCOL_SUCCESS)
			{
				m_huart2.tx_mem.length = m_huart2.tx_buf_alias.len;
				rc = cobs_encode_single_buffer(&m_huart2.tx_mem);
				if(rc == COBS_SUCCESS)
				{
					m_uart_dma_transmit(&m_huart2);
				}
			}
		}
		m_huart2.rx_decoded.length = 0;
	}
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
	size_t num_writes = size/WRITE_SIZE;	//modulo check already done pre-call, so we can safely ignore it

	HAL_FLASH_Unlock();
	uint32_t error = HAL_FLASH_ERROR_NONE;
	size_t srcidx = 0;
	uint32_t dest_addr = (uint32_t)(dest);	//HAL casts pointer to uint32_t - equal to uintptr_t but locked to our target. acceptable
	for(size_t i = 0; i < num_writes; i++)
	{

		//little endian data load into payload word
		uint64_t data = 0;
		for(int i = 0; i < sizeof(uint64_t); i++)
		{
			data |= ((uint64_t)src[srcidx++]) << (i*8);
		}

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, dest_addr, data) == HAL_OK)
		{
			dest_addr += WRITE_SIZE;
		}
		else
		{
			/* Error occurred while writing data in Flash memory*/
			error = HAL_FLASH_GetError ();
			break;
		}
	}

	HAL_FLASH_Lock();

	if(error != HAL_FLASH_ERROR_NONE)
	{
		return DARTT_BL_ERASE_BLOCKED;
	}
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages)
{

	HAL_FLASH_Unlock();
	uint32_t error = HAL_FLASH_ERROR_NONE;
	FLASH_EraseInitTypeDef EraseInitStruct = {};
	uint32_t PAGEError;
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = FLASH_BANK_1;	//on G431, there is only one bank per page
	EraseInitStruct.Page = erase_page;
	EraseInitStruct.NbPages     = erase_num_pages;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
	{
		/*Error occurred while page erase.*/
		error = HAL_FLASH_GetError ();
		//consider writing error to working buffer?
	}
	HAL_FLASH_Lock();


	if(error != HAL_FLASH_ERROR_NONE)
	{
		return DARTT_BL_ERASE_BLOCKED;
	}
	return DARTT_BL_SUCCESS;
}
