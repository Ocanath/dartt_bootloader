#include "dartt_flasher.h"
#include "callbacks.h"
#include <string.h>
#include <fstream>
#include "milliseconds.h"
#include "dartt_bl.h"

DarttFlasher::DarttFlasher(unsigned char addr)
{
	tx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	rx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	ds = (dartt_sync_t){};
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
	ds.timeout_ms = 1000;

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

	erase_region.buf = (unsigned char *)(&bootloader_control.erase_page);
	erase_region.size = 2*sizeof(uint32_t);

	timeout = 2000;
	target_pointer_size = 0;
	initialized = false;
	attr_cpy = (dartt_bl_attributes_t){};
	app_start = 0;
	flash_start = 0;
}

DarttFlasher::~DarttFlasher()
{
	delete[] tx_buf_mem;
	delete[] rx_buf_mem;
	ser.disconnect();
}



int DarttFlasher::init(void)
{
	if(initialized)
	{
		return ERROR_ALREADY_INITIALIZED;
	}
	int rc = 0;
	rc = dartt_read_multi(&ds.ctl_base, &ds);
	if(rc != FLASHER_SUCCESS)
	{
		return rc;
	}
	attr_cpy = bootloader_periph.attr;
	rc = get_target_pointer_size();
	if(rc != FLASHER_SUCCESS)
	{
		return rc;
	}

	app_start = get_pointer(GET_APPLICATION_START_ADDR);
	if(app_start == 0)
	{
		return ERROR_PTR_RETRIEVAL_FAILED;
	}
	flash_start = get_pointer(GET_FLASH_BASE_ADDR);
	if(flash_start == 0)
	{
		return ERROR_PTR_RETRIEVAL_FAILED;
	}

	initialized = true;	
	return FLASHER_SUCCESS;	
}



int DarttFlasher::poll_action_flags(uint32_t timeout_ms)
{
	uint32_t start_ts = get_tick32();

	while(get_tick32() - start_ts < timeout_ms)
	{
		int rc = dartt_read_multi(&action_flags, &ds);
		if(rc != DARTT_PROTOCOL_SUCCESS && rc != DARTT_READ_CALLBACK_TIMEOUT)
		{
			//read timeouts are allowed on poll, because some operations block the event loop from handling a read request properly
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
	int rc = dartt_write_multi(&action_flags, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	return poll_action_flags(timeout);
}

int DarttFlasher::get_version(std::string & version)
{
	int rc = write_action_flag(GET_VERSION_HASH);
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

/*
Helper to get pointer. Flag must be either get working addr or get app start addr.
*/
uintptr_t DarttFlasher::get_pointer(uint32_t flag)
{
	if(flag != GET_WORKING_ADDR && flag != GET_APPLICATION_START_ADDR && flag != GET_FLASH_BASE_ADDR)
	{
		return 0;
	}

	bootloader_periph.working_size = 0;
	int rc = write_action_flag(flag);
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
Helper to get the working size. Loads a pointer and then stores the size.
*/
int DarttFlasher::get_target_pointer_size(void)
{
	uintptr_t tmp = get_pointer(GET_APPLICATION_START_ADDR);
	if(tmp == 0)
	{
		return ERROR_POINTERSIZE_NOT_LOADED;
	}
	target_pointer_size = (size_t)(bootloader_periph.working_size);
	if(target_pointer_size == 0)
	{
		return ERROR_POINTERSIZE_NOT_LOADED;
	}
	if(target_pointer_size > sizeof(bootloader_control.working_buffer))
	{
		return ERROR_MEMORY_OVERRUN;
	}
	return FLASHER_SUCCESS;
}

/*
Helper to update the working buffer.

The size parameter of the controller is used to make the operation more efficient.

Order of operations:

This triggers:
1. read of the relevant range, determined by write size. 
2. write of only bytewise deltas betwee ctl and periph
3. readback of written deltas to confirm they were written

*/
int DarttFlasher::write_working_buffer(void)
{
	dartt_mem_t range = working_buffer;	//make a local copy of the working buffer
	range.size = sizeof(bootloader_control.working_size) + (size_t)bootloader_control.working_size;

	int rc = dartt_read_multi(&working_buffer, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	rc = dartt_sync(&working_buffer, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	return FLASHER_SUCCESS;
}

int DarttFlasher::set_working_pointer(uintptr_t pointer)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	if(target_pointer_size > sizeof(uintptr_t))
	{
		return ERROR_POINTERSIZE_TOO_LARGE;
	}
	bootloader_control.working_size = target_pointer_size;
	for(size_t i = 0; i < target_pointer_size; i++)
	{
		int shift = (i*8);
		bootloader_control.working_buffer[i] = (pointer & (0xFF << shift)) >> shift;
	}
	int rc = write_working_buffer();
	if(rc != FLASHER_SUCCESS)
	{
		return rc;
	}
	return write_action_flag(SET_WORKING_ADDR);	
}

int DarttFlasher::get_page_idx_of_pointer(uintptr_t pointer, uint32_t & page_idx)
{
	if(pointer < flash_start)
	{
		return ERROR_INVALID_ARGUMENT;
	}
	uintptr_t offset = pointer - flash_start;
	uintptr_t page_size = ((uintptr_t)attr_cpy.page_size);
	page_idx = (uint32_t)(offset/page_size);
	uintptr_t rem = offset % page_size;
	if(rem != 0)
	{
		page_idx++;	//any remainder means we have to erase that final page
	}

	return FLASHER_SUCCESS;
}

int DarttFlasher::erase_blob(uintptr_t start, size_t len)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	int rc = get_page_idx_of_pointer(start, bootloader_control.erase_page);
	if(rc != FLASHER_SUCCESS){return rc;}
	uint32_t last_page_idx = 0;
	rc = get_page_idx_of_pointer(start + len, last_page_idx);
	if(rc != FLASHER_SUCCESS){return rc;}
	bootloader_control.erase_num_pages = last_page_idx - bootloader_control.erase_page;
	printf("Start page: %d\n", bootloader_control.erase_page);
	printf("Num pages: %d\n", bootloader_control.erase_num_pages);	
	if(bootloader_control.erase_num_pages == 0)
	{
		return ERROR_NOTHING_TO_ERASE;
	}

	/*Write out the control parameters before dispatch*/
	rc = dartt_write_multi(&erase_region, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	rc = dartt_read_multi(&erase_region, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	if(bootloader_control.erase_num_pages != bootloader_periph.erase_num_pages 
		|| bootloader_control.erase_page != bootloader_periph.erase_page)
	{
		return ERROR_LOAD_FAILED;
	}

	//dispatch erase
	return write_action_flag(ERASE_PAGES);
}

int DarttFlasher::mass_erase(void)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	int rc = 0;
	rc = get_page_idx_of_pointer(app_start, bootloader_control.erase_page);
	if(bootloader_control.erase_page > attr_cpy.num_pages)
	{
		return ERROR_ATTR_INVALID;
	}
	bootloader_control.erase_num_pages = (attr_cpy.num_pages - bootloader_control.erase_page);

	//for small buffers, write then read for confirmation is likely better than read + sync, since the number of deltas is most likely the worst case
	rc = dartt_write_multi(&erase_region, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	rc = dartt_read_multi(&erase_region, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	if(bootloader_control.erase_num_pages != bootloader_periph.erase_num_pages 
		|| bootloader_control.erase_page != bootloader_periph.erase_page)
	{
		return ERROR_LOAD_FAILED;
	}
	return write_action_flag(ERASE_PAGES);
}

/*
	Helper for writing raw binary data to the target
*/
int DarttFlasher::write_bin(const std::string & path)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if(!file.is_open())
	{
		return ERROR_INVALID_ARGUMENT;
	}
	printf("Writing file %s\n", path.c_str());
	size_t len = (size_t)file.tellg();
	file.seekg(0);
	int rc;

	printf("Flash start at location 0x%lX\n", flash_start);
	rc = set_working_pointer(app_start);
	if(rc != FLASHER_SUCCESS){return rc;}
	uintptr_t working_addr = get_pointer(GET_WORKING_ADDR);	//redundant - set working pointer reads back changes. However it's kind of nice to confirm read works properly
	if(app_start != working_addr)
	{return ERROR_PTR_RETRIEVAL_FAILED;}

	//pre-erase the whole target region for incremental write
	rc = erase_blob(app_start, len);	
	if(rc != FLASHER_SUCCESS){return rc;}
	
	size_t max_write_size = (sizeof(bootloader_control.working_buffer)/attr_cpy.write_size)*attr_cpy.write_size;	//floor division and reinflation yields max write size. Since working buffer is 64, this probably always yields 64
	size_t num_max_writes = len/max_write_size;
	size_t nbytes_remainder = len % max_write_size;	//number of bytes in the tail/final write. must pad up to write size

	for(size_t i = 0; i < num_max_writes; i++)
	{
		file.read(reinterpret_cast<char*>(bootloader_control.working_buffer), max_write_size);
		bootloader_control.working_size = max_write_size;
		rc = write_working_buffer();
		if(rc != FLASHER_SUCCESS)
		{
			return rc;
		}
		rc = write_action_flag(WRITE_BUFFER);
		if(rc != FLASHER_SUCCESS){return rc;}	

		working_addr += max_write_size;
		rc = set_working_pointer(working_addr);
		if(rc != FLASHER_SUCCESS){return rc;}	//this uses sync which is redundant
	}
	if(nbytes_remainder != 0)
	{
		size_t padded_size = ((nbytes_remainder + attr_cpy.write_size - 1)/attr_cpy.write_size)*attr_cpy.write_size;
		memset(bootloader_control.working_buffer, 0xFF, padded_size);
		file.read(reinterpret_cast<char*>(bootloader_control.working_buffer), nbytes_remainder);
		bootloader_control.working_size = padded_size;
		rc = write_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		rc = write_action_flag(WRITE_BUFFER);
		if(rc != FLASHER_SUCCESS){return rc;}
	}

	return FLASHER_SUCCESS;
}
