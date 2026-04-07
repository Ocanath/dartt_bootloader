/*
 * dartt_bl_stubs.c
 *
 *  Created on: Apr 7, 2026
 *      Author: ocanath
 */
#include "dartt_bl_stubs.h"
#include "cobs.h"
#include "dartt.h"
#include "m_dma_uart.h"

const unsigned char * flash_base_addr__ = (const unsigned char *)(0x08000000);	//TODO: DELETE THIS, GENERATE VIA LINKER
const unsigned char * application_start_addr__ = (const unsigned char *)(0x08003000);	//TODO: DELETE THIS, GENERATE VIA LINKER

dartt_bl_t gl_bootloader = {};
dartt_mem_t bootloader_alias = {
		.buf = (unsigned char*)(&gl_bootloader),
		.size = sizeof(gl_bootloader)
};

uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
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
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_start_application(dartt_bl_t * pbl)
{
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
