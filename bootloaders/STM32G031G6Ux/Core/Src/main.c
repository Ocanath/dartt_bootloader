#include "init.h"
#include "dartt.h"
#include "cobs.h"
#include "uart_mem.h"







/*
 * Event handler for dartt.
 * Designed to be a polling handler in the primary event loop, and synchronous with data loads in the dartt_map
 * I.e. do not call this in an interrupt handler
 *
 * TODO: Consider passing primary address and memory alias as function parameters, and then using this as a fully portable function in its own TU for vendorability
 * Steps for this:
 * 		1. Update signature with motor_address, misc_address, dartt_map_alias (dartt_mem_t *)
 * 		2. remove encoder specific theta_rel_14b() callsites. Move these to main event loop. Enable full circular dma and no interrupts. Test to make sure not choked
 * 		3. Move to a dma_uart_dartt TU
 * 		4. Replace motor parser with a weak function which has the map alias (dartt_mem_t *) and (dma_uart_t*) in the signature - add a note that the dartt_mem_t is a 'context pointer' you can cast the ->buf to the defined struct map type for member access if desired
 * */
int handle_serial_dartt(dma_uart_t * uart, unsigned char misc_address, dartt_mem_t * dp_alias)
{
	if(uart->rx_decoded.length == 0)
	{
		return 1;	//TODO: enumerate codes. This is a 'skip, no new message'
	}

	//both dartt misc and dartt motor messages have [address][payload][crc] for TYPE_SERIAL so F2P is correct for CRC check and payload parsing
	payload_layer_msg_t * pld = &uart->rx_pld_msg;
	int rc = dartt_frame_to_payload(&uart->rx_decode_alias, TYPE_SERIAL_MESSAGE, PAYLOAD_ALIAS, pld);
	uart->rx_decoded.length = 0;	//unconditionally invalidate decoded cobs frame after F2P call to avoid repeated f2p calls/failures
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}

	if(pld->address == misc_address)
	{
		rc = dartt_parse_general_message(pld, TYPE_SERIAL_MESSAGE, dp_alias, &uart->tx_buf_alias);
		if(rc != DARTT_PROTOCOL_SUCCESS)
		{
			return rc;
		}
		if(uart->tx_buf_alias.len != 0)
		{
			uart->tx_mem.length = uart->tx_buf_alias.len;
			rc = cobs_encode_single_buffer(&uart->tx_mem);
			if(rc != COBS_SUCCESS)
			{
				return rc;
			}
			m_uart_dma_transmit(uart);
		}
		rc = DARTT_PROTOCOL_SUCCESS;	//this will get compiled out - kept for function contract clarity
	}
	else
	{
		rc = DARTT_ADDRESS_FILTERED;
	}

	return rc;
}

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);

	while (1)
	{

	}
}

