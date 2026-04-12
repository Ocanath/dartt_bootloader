#ifndef DARTT_FLASHER_H
#define DARTT_FLASHER_H

#include "dartt_sync.h"
#include "dartt_bl.h"
#include "serial.h"

class DarttFlasher
{
	public:
		DarttFlasher(unsigned char addr);
		~DarttFlasher();
		
	private:
		Serial ser;

		dartt_bl_t bootloader_control;
		dartt_bl_t bootloader_periph;

		dartt_sync_t ds;
		unsigned char * tx_buf_mem;
		unsigned char * rx_buf_mem;
};

#endif
