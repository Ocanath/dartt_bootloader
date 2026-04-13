#ifndef DARTT_FLASHER_H
#define DARTT_FLASHER_H

#include "dartt_sync.h"
#include "dartt_bl.h"
#include "serial.h"
#include <string.h>


class DarttFlasher
{
	public:
		DarttFlasher(unsigned char addr);
		~DarttFlasher();
		Serial ser;

		int get_version(std::string & version);


		uint32_t timeout;


		enum {
			FLASHER_SUCCESS = 0,
			ERROR_TIMEOUT = -100,
			ERROR_VERSION_RETRIEVAL_FAILED = -101
		};

	private:
		unsigned char * tx_buf_mem;
		unsigned char * rx_buf_mem;

		dartt_bl_t bootloader_control;
		dartt_bl_t bootloader_periph;

		dartt_sync_t ds;

		dartt_mem_t action_flags;
		dartt_mem_t working_buffer;

		int poll_action_flags(uint32_t timeout_ms);
		int write_action_flag(uint32_t flag);	//helper to load control with proper flags before dispatch. called before poll
};

#endif

