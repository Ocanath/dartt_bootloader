#include "dartt_flasher.h"
#include "callbacks.h"
#include <string.h>
#include "milliseconds.h"
#include "dartt_bl.h"

DarttFlasher::DarttFlasher(unsigned char addr)
{
	tx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	rx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	ds.address = addr;
	ds.base_offset = 0;
	ds.msg_type = TYPE_SERIAL_MESSAGE;
	ds.tx_buf.buf = tx_buf_mem;
	ds.tx_buf.size = _DARTT_SERIAL_BUFFER_SIZE - _DARTT_NUM_BYTES_COBS_OVERHEAD;		//DO NOT CHANGE. This is for a good reason. See above note
	ds.tx_buf.len = 0;
	ds.rx_buf.buf = rx_buf_mem;
	ds.rx_buf.size = _DARTT_SERIAL_BUFFER_SIZE - _DARTT_NUM_BYTES_COBS_OVERHEAD;	//DO NOT CHANGE. This is for a good reason. See above note
	ds.rx_buf.len = 0;
	ds.blocking_tx_callback = &_tx_blocking_callback;
	ds.user_context_tx = (void*)(&ser);
	ds.blocking_rx_callback = &_rx_blocking_callback;
	ds.user_context_rx = (void*)(&ser);
	ds.timeout_ms = 100;

	ds.ctl_base.buf = (unsigned char *)(&bootloader_control);
	ds.ctl_base.size = sizeof(bootloader_control);

	ds.periph_base.buf = (unsigned char *)(&bootloader_periph);
	ds.periph_base.size = sizeof(bootloader_periph);

	memset(ds.ctl_base.buf, 0, ds.ctl_base.size);
	memset(ds.periph_base.buf, 0, ds.periph_base.size);

	action_flags.buf = (unsigned char *)(&bootloader_control.action_flag);
	action_flags.size = 2*sizeof(uint32_t);

	working_buffer.buf = (unsigned char *)(&bootloader_control.working_size);
	working_buffer.size = sizeof(bootloader_control.working_size)+sizeof(bootloader_control.working_buffer);

	timeout = 2000;
}

DarttFlasher::~DarttFlasher()
{
	delete[] tx_buf_mem;
	delete[] rx_buf_mem;
	ser.disconnect();
}

int DarttFlasher::poll_action_flags(uint32_t timeout_ms)
{
	uint32_t start_ts = get_tick32();

	while(get_tick32() - start_ts < timeout_ms)
	{
		int rc = dartt_read_multi(&action_flags, &ds);
		if(rc != DARTT_PROTOCOL_SUCCESS)
		{
			return rc;
		}
		int32_t action_status = (int32_t)bootloader_periph.action_status;
		if(bootloader_periph.action_flag == 0)
		{
			if(action_status == 0)
			{
				return FLASHER_SUCCESS;
			}
			else 
			{
				return (int)action_status;
			}
		}
	}
	return	ERROR_TIMEOUT;
}

int DarttFlasher::write_action_flag(uint32_t flag)
{
	bootloader_control.action_flag = flag;
	bootloader_periph.action_flag = bootloader_control.action_flag;	//load to peripheral copy so poll works without issues
	bootloader_periph.action_status = 0;
	return dartt_write_multi(&action_flags, &ds);
}


int DarttFlasher::get_version(std::string & version)
{
	int rc = write_action_flag(GET_VERSION_HASH);
	if(rc != FLASHER_SUCCESS)
	{
		return rc;
	}

	rc = poll_action_flags(timeout);
	if(rc != FLASHER_SUCCESS)
	{
		return rc;
	}


	rc = dartt_read_multi(&working_buffer, &ds);
	if(bootloader_periph.working_size < 7)
	{
		return ERROR_VERSION_RETRIEVAL_FAILED;
	}

	version.assign(
		(const char*)(bootloader_periph.working_buffer),
		bootloader_periph.working_size
	);
	return 0;
}

uintptr_t DarttFlasher::get_start_pointer(void)
{
	bootloader_periph.working_size = 0;
	int rc = write_action_flag(GET_APPLICATION_START_ADDR);
	if(rc != FLASHER_SUCCESS)
	{
		return 0;
	}

	rc = poll_action_flags(timeout);
	if(rc != FLASHER_SUCCESS)
	{
		return 0;
	}

	rc = dartt_read_multi(&working_buffer, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return 0;
	}

	uintptr_t app_start = 0;
	for(size_t i = 0; i < bootloader_periph.working_size; i++)
	{
		app_start |= (uintptr_t)(bootloader_periph.working_buffer[i]) << (i*8);
	}
	
	return app_start;
}

/*
	Helper for writing raw binary data to the target
*/
int DarttFlasher::write_bin(const unsigned char * bin, size_t len)
{
	uintptr_t app_start = get_start_pointer();
	if(app_start == 0)
	{
		return ERROR_PTR_RETRIEVAL_FAILED;
	}
	printf("0x%lX\n", app_start);
}
