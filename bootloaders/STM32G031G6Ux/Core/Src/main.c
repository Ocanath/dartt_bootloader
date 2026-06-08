#include "init.h"
#include "fds.h"
#include "dartt_map.h"
#include "dartt.h"
#include "cobs.h"
#include "uart_mem.h"
#include "tle_encoder.h"




dartt_map_t gl_dp = {
		.angle = 0,
		.dma_adc_raw = {0},
		.fds = {
				.address = 0,
				.sin_min = 0,
				.sin_max = 0,
				.cos_min = 0,
				.cos_max = 0,
				.oerr = 0,
				.baud = 921600
		},
		.tick = 0
};

dartt_mem_t gl_dp_alias = {
		.buf = (unsigned char *)(&gl_dp),
		.size = sizeof(gl_dp)
};

enum {NO_ACTION = 0, FS_SAVE = 1, RESTART = 2, BOOTLOAD = 6, LED_ON = 3, LED_OFF = 4};

/*
 * Helper function to load all the fds_mp params
 * Must go before CAN init for can ID to work properly
 * */
void load_flash_params(void)
{
	if(is_page_empty(sizeof(gl_dp.fds)/sizeof(uint32_t)) == 0)
		m_read_flash((uint32_t*)&gl_dp.fds,sizeof(fds_t)/sizeof(uint32_t));
	else
		m_write_flash((uint64_t*)&gl_dp.fds,sizeof(fds_t)/sizeof(uint64_t));
}

void fs_write_handler(void)
{
	if(gl_dp.action_flag == FS_SAVE)
	{
		m_write_flash((uint64_t*)&gl_dp.fds,sizeof(fds_t)/sizeof(uint64_t));
	}
}

void reset_handler(void)
{
	if(gl_dp.action_flag == RESTART)
	{
		NVIC_SystemReset();
	}
}

void bootload_handler(void)
{
	if(gl_dp.action_flag == BOOTLOAD)
	{
		//Do ram stuff, magic word
		NVIC_SystemReset();
	}
}

void led_handler(void)
{
	if(gl_dp.action_flag == LED_ON)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);
	}
	else if(gl_dp.action_flag == LED_OFF)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
	}
}



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
int handle_serial_dartt(dma_uart_t * uart, unsigned char misc_address)
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
		rc = dartt_parse_general_message(pld, TYPE_SERIAL_MESSAGE, &gl_dp_alias, &uart->tx_buf_alias);
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
	else if(pld->address == gl_dp.fds.address)
	{
		dartt_buffer_t * txb = &uart->tx_buf_alias;
		txb->len = 0;
		txb->buf[txb->len++] = MASTER_MOTOR_ADDRESS;
		txb->buf[txb->len++] = gl_dp.angle & 0xFF;
		txb->buf[txb->len++] = ((gl_dp.angle & 0xFF00) >> 8);
		rc = append_crc(txb);
		if(rc != DARTT_PROTOCOL_SUCCESS)
		{
			return rc;
		}
		uart->tx_mem.length = txb->len;
		rc = cobs_encode_single_buffer(&uart->tx_mem);
		if(rc != COBS_SUCCESS)
		{
			return rc;
		}
		m_uart_dma_transmit(uart);
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
	load_flash_params();	//read, write default. load before UART
	unsigned char dartt_misc_address = dartt_get_complementary_address((uint32_t)gl_dp.fds.address);
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_ADC1_Init();
	HAL_ADC_Start_DMA(&hadc1, (uint32_t * )gl_dp.dma_adc_raw, NUM_ADC);
	hdma_adc1.Instance->CCR &= ~(1 << 1);	//disable transfer complete interrupt
	hdma_adc1.Instance->CCR &= ~(1 << 2);	//disable half transfer complete interrupt
	MX_USART2_UART_Init();
	eval_encoder_params();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);

	while (1)
	{
		gl_dp.angle = theta_rel_14b();

		/*Handle DARTT over UART*/
		handle_serial_dartt(&m_huart2, dartt_misc_address);

		if(gl_dp.action_flag != NO_ACTION)
		{
			reset_handler();
			bootload_handler();
			led_handler();
			fs_write_handler();

			gl_dp.action_flag = NO_ACTION;
		}
	}
}

