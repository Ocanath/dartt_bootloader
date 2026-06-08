#include "main.h"
#include "init.h"
#include "dartt.h"
#include "dartt_bl.h"
#include "uart_mem.h"

extern dartt_bl_t gl_bootloader;


int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);

	uint32_t led_ts = 0;

	dartt_bl_init(&gl_bootloader);
	while (1)
	{
		dartt_bl_event_handler(&gl_bootloader);

		uint32_t tick = HAL_GetTick();
		if(tick - led_ts > 333)
		{
			led_ts = tick;
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_7);
		}
	}
}

