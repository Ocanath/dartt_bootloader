#include "dartt_flasher.h"
#include "callbacks.h"
#include <string.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "milliseconds.h"
#include "dartt_bl.h"

DarttFlasher::DarttFlasher(unsigned char addr)
{
	tx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	rx_buf_mem = new unsigned char[_DARTT_SERIAL_BUFFER_SIZE];
	ds = {};
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

	timeout = 5000;
	target_pointer_size = 0;
	initialized = false;
	attr_cpy = {};
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
		return ERROR_FLASHER_ALREADY_INITIALIZED;
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
	return	ERROR_FLASHER_TIMEOUT;
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

int DarttFlasher::verify_app(uint32_t crc32)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}

	dartt_mem_t dm_crc32 = 
	{
		.buf = (unsigned char *)(&bootloader_control.fds.application_crc32),
		.size = sizeof(uint32_t)
	};	
	int rc = write_action_flag(GET_CRC32);
	if(rc != FLASHER_SUCCESS){return rc;}
	rc = dartt_read_multi(&dm_crc32, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	if(crc32 != bootloader_periph.fds.application_crc32)
	{
		return ERROR_VERIFY_FAILED;
	}
	return FLASHER_SUCCESS;	//match, return happy
}

int DarttFlasher::get_bin_crc(const std::string & path, uint32_t & crc)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if(!file.is_open())
	{
		return ERROR_INVALID_ARGUMENT;
	}
	size_t len = (size_t)file.tellg();
	file.seekg(0);


	size_t i;
	int j;
	uint32_t byte, mask;

	i = 0;
	crc = 0xFFFFFFFF;
	while (i < len)
	{
		unsigned char raw = 0;
		file.read((char*)(&raw), 1);

		byte = (uint32_t)raw;// Get next byte.
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--)
		{// Do eight times.
			mask = (crc & 1) ? 0xFFFFFFFFU : 0U;
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	crc = ~crc;
	return FLASHER_SUCCESS;
}

int DarttFlasher::read_working_buffer(void)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	/*
	Target location of just the working size register
	*/
	dartt_mem_t dbl_workingsize = 
	{
		.buf = (unsigned char *)(&bootloader_control.working_size),
		.size = sizeof(bootloader_control.working_size)
	};

	if(bootloader_control.working_size == 0)	//forbid size 0 - this is caught in the bootloader anyway, but we use 0 as a sentinel for readback verification
	{
		return ERROR_INVALID_ARGUMENT;	//placeholder? maybe a better return value for this?
	}

	memset(bootloader_periph.working_buffer, 0, sizeof(bootloader_periph.working_buffer));
	bootloader_periph.working_size = 0;

	int rc = dartt_write_multi(&dbl_workingsize, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	rc = write_action_flag(READ_BUFFER);
	rc = dartt_read_multi(&working_buffer, &ds);
	if(bootloader_periph.working_size != bootloader_control.working_size)
	{
		return ERROR_READ_FAILED;
	}
	return FLASHER_SUCCESS;
}

int DarttFlasher::readback_verification(const std::string & path, uintptr_t start_ptr)
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
	printf("Verifying file %s\n", path.c_str());
	size_t len = (size_t)file.tellg();
	file.seekg(0);
	int rc;
	if(start_ptr == 0)
	{
		start_ptr = app_start;
	}
	rc = set_working_pointer(start_ptr);
	if(rc != FLASHER_SUCCESS){return rc;}
	uintptr_t working_addr = get_pointer(GET_WORKING_ADDR);	//redundant - set working pointer reads back changes. However it's kind of nice to confirm read works properly
	if(start_ptr != working_addr)
	{return ERROR_PTR_RETRIEVAL_FAILED;}


	size_t max_read_size = sizeof(bootloader_control.working_buffer);
	size_t num_max_reads = len/max_read_size;
	size_t nbytes_remainder = len % max_read_size;	//number of bytes in the tail/final read
	printf("Verifying...\n");
	unsigned char cmp_buf[sizeof(bootloader_control.working_buffer)] = {};

	for(size_t i = 0; i < num_max_reads; i++)
	{
		printf("Reading chunk %lu, %lu bytes\n", (unsigned long)i, (unsigned long)max_read_size);

		/*Read a chunk from the file*/
		memset(cmp_buf, 0, sizeof(cmp_buf));
		file.read(reinterpret_cast<char*>(cmp_buf), max_read_size);

		/*Read the corresponding chunk from the target*/
		bootloader_control.working_size = max_read_size;
		rc = read_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		
		rc = memcmp(cmp_buf, bootloader_periph.working_buffer, max_read_size);
		if(rc != 0)
		{
			printf("Verify FAILED: First difference found between 0x%lX and 0x%lX\n", working_addr, working_addr + max_read_size);
			return ERROR_VERIFY_FAILED;
		}

		working_addr += max_read_size;
		rc = set_working_pointer(working_addr);
		if(rc != FLASHER_SUCCESS){return rc;}	//this uses sync which is redundant
	}
	if(nbytes_remainder != 0)
	{
		printf("Reading final chunk %lu bytes\n", (unsigned long)nbytes_remainder);

		/*Read a chunk from the file*/
		memset(cmp_buf, 0, sizeof(cmp_buf));
		file.read(reinterpret_cast<char*>(cmp_buf), nbytes_remainder);

		/*Read the corresponding chunk from the target*/
		bootloader_control.working_size = nbytes_remainder;
		rc = read_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		
		rc = memcmp(cmp_buf, bootloader_periph.working_buffer, nbytes_remainder);
		if(rc != 0)
		{
			printf("Verify FAILED: First difference found between 0x%lX and 0x%lX\n", working_addr, working_addr + nbytes_remainder);
			return ERROR_VERIFY_FAILED;
		}
	}
	return FLASHER_SUCCESS;
}

int DarttFlasher::read_to_file(const std::string & path, uintptr_t start_ptr, size_t len)
{
	if(initialized == false)
	{
		return ERROR_NOT_INITIALIZED;
	}
	std::ofstream file(path, std::ios::binary);
	if(!file.is_open())
	{
		return ERROR_INVALID_ARGUMENT;
	}
	if(start_ptr == 0)
	{
		start_ptr = app_start;
	}
	if(len == 0)
	{
		uintptr_t flash_end = flash_start + (uintptr_t)attr_cpy.num_pages * (uintptr_t)attr_cpy.page_size;
		len = (size_t)(flash_end - start_ptr);
	}
	int rc = set_working_pointer(start_ptr);
	if(rc != FLASHER_SUCCESS){return rc;}
	uintptr_t working_addr = get_pointer(GET_WORKING_ADDR);
	if(start_ptr != working_addr){return ERROR_PTR_RETRIEVAL_FAILED;}

	size_t max_read_size = sizeof(bootloader_control.working_buffer);
	size_t num_max_reads = len / max_read_size;
	size_t nbytes_remainder = len % max_read_size;

	for(size_t i = 0; i < num_max_reads; i++)
	{
		printf("Reading chunk %lu, %lu bytes\n", (unsigned long)i, (unsigned long)max_read_size);

		bootloader_control.working_size = max_read_size;
		rc = read_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		file.write((char*)bootloader_periph.working_buffer, max_read_size);
		working_addr += max_read_size;
		rc = set_working_pointer(working_addr);
		if(rc != FLASHER_SUCCESS){return rc;}
	}
	if(nbytes_remainder != 0)
	{
		printf("Reading final chunk, %lu bytes\n", (unsigned long)nbytes_remainder);
		bootloader_control.working_size = nbytes_remainder;
		rc = read_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		file.write((char*)bootloader_periph.working_buffer, nbytes_remainder);
	}
	return FLASHER_SUCCESS;
}

/*
	Helper for writing raw binary data to the target
*/
int DarttFlasher::write_bin(const std::string & path, bool verify, uintptr_t start_ptr)
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
	printf("File size %lu bytes\n", (unsigned long)len);
	file.seekg(0);
	int rc;

	printf("Flash start at location 0x%lX\n", flash_start);
	if(start_ptr == 0)
	{
		start_ptr = app_start;
	}
	rc = set_working_pointer(start_ptr);
	if(rc != FLASHER_SUCCESS){return rc;}
	uintptr_t working_addr = get_pointer(GET_WORKING_ADDR);	//redundant - set working pointer reads back changes. However it's kind of nice to confirm read works properly
	if(start_ptr != working_addr)
	{return ERROR_PTR_RETRIEVAL_FAILED;}

	//pre-erase the whole target region for incremental write
	printf("Pre-erasing application region, starting at 0x%lX...\n", (unsigned long)app_start);
	rc = erase_blob(start_ptr, len);	
	if(rc != FLASHER_SUCCESS){return rc;}
	printf("Success!\n");
	
	size_t max_write_size = (sizeof(bootloader_control.working_buffer)/attr_cpy.write_size)*attr_cpy.write_size;	//floor division and reinflation yields max write size. Since working buffer is 64, this probably always yields 64
	size_t num_max_writes = len/max_write_size;
	size_t nbytes_remainder = len % max_write_size;	//number of bytes in the tail/final write. must pad up to write size

	for(size_t i = 0; i < num_max_writes; i++)
	{
		printf("Writing chunk %lu, %lu bytes\n", (unsigned long)i, (unsigned long)max_write_size);
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
		printf("Writing final chunk %lu bytes", (unsigned long)padded_size);
		if(padded_size == nbytes_remainder)
		{
			printf("\n");
		}
		else
		{
			printf("(padded from %lu)\n", (unsigned long)nbytes_remainder);
		}
		memset(bootloader_control.working_buffer, 0xFF, padded_size);
		file.read(reinterpret_cast<char*>(bootloader_control.working_buffer), nbytes_remainder);
		bootloader_control.working_size = padded_size;
		rc = write_working_buffer();
		if(rc != FLASHER_SUCCESS){return rc;}
		rc = write_action_flag(WRITE_BUFFER);
		if(rc != FLASHER_SUCCESS){return rc;}
	}

	printf("Writing appsize...\n");
	dartt_mem_t dm_appsize = 
	{
		.buf = (unsigned char *)(&bootloader_control.fds.application_size),
		.size = sizeof(uint32_t)
	};
	bootloader_control.fds.application_size = len;
	rc = dartt_write_multi(&dm_appsize, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	rc = dartt_read_multi(&dm_appsize, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS){return rc;}
	if(bootloader_periph.fds.application_size != bootloader_control.fds.application_size)
	{
		return ERROR_LOAD_FAILED;
	}
	
	//finally, save app size so the crc32 calc is correct
	rc = write_action_flag(SAVE_SETTINGS);
	if(rc != FLASHER_SUCCESS){return rc;}

	printf("Flashing Done!\n");

	if(verify)
	{
		if(start_ptr == app_start)
		{
			uint32_t crc32 = 0;
			rc = get_bin_crc(path, crc32);
			if(rc != FLASHER_SUCCESS){return ERROR_VERIFY_FAILED;}
			rc = verify_app(crc32);
			if(rc == FLASHER_SUCCESS)
			{
				printf("CRC Verify Success!\n");
			}
			else
			{
				printf("Error: Verification Failed!\n");
				return rc;
			}
		}
		else
		{
			rc = readback_verification(path, start_ptr);
			if(rc == FLASHER_SUCCESS)
			{
				printf("Readback Verify Success!\n");
			}
			return rc;
		}
	}
	return FLASHER_SUCCESS;
}

int DarttFlasher::start_app(void)
{
	bootloader_control.action_flag = START_APPLICATION;
	bootloader_periph.action_flag = bootloader_control.action_flag;	//load to peripheral copy so poll works without issues
	bootloader_periph.action_status = 0;
	int rc = dartt_write_multi(&action_flags, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	return FLASHER_SUCCESS;
}


int DarttFlasher::update_target_address(unsigned char addr)
{
	int rc = 0;
	dartt_mem_t module_number_slice = { 
		(unsigned char *)(&bootloader_control.fds.module_number),
		sizeof(uint32_t)
	};
	bootloader_control.fds.module_number = addr;
	rc = dartt_write_multi(&module_number_slice, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	//possibly may want to add a delay here
	ds.address = addr;
	rc = dartt_read_multi(&module_number_slice, &ds);
	if(rc != DARTT_PROTOCOL_SUCCESS)
	{
		return rc;
	}
	if(bootloader_periph.fds.module_number != addr)
	{
		return ERROR_LOAD_FAILED;
	}
	return write_action_flag(SAVE_SETTINGS);
}

int DarttFlasher::set_target_bootmode(bool enabled)
{
	dartt_mem_t boot_mode_slice = 
	{
		(unsigned char *)(&bootloader_control.fds.boot_mode),
		sizeof(uint32_t)
	};
	int rc = 0;
	if(enabled)
	{
		bootloader_control.fds.boot_mode = DARTT_BL_START_PROGRAM_KEY;	//set to key
		bootloader_periph.fds.boot_mode = 0;	//use sync hack for api readback verification
		rc = dartt_sync(&boot_mode_slice, &ds);
	}
	else
	{
		bootloader_control.fds.boot_mode = 0;	//set to not the key
		bootloader_periph.fds.boot_mode = 0xFFFFFFFF;	//use sync hack for api readback verification
		rc = dartt_sync(&boot_mode_slice, &ds);
	}
	if(rc != DARTT_PROTOCOL_SUCCESS) return rc;
	return write_action_flag(SAVE_SETTINGS);
}
