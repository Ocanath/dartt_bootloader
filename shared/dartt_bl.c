#include "dartt_bl.h"
#include "dartt_bl_stubs.h"
#include "checksum.h"
#include <stdbool.h>

uint32_t dartt_bl_read_mem(dartt_bl_t * pbl);
uint32_t dartt_bl_get_crc32(dartt_bl_t * pbl);
uint32_t dartt_bl_load_ptr_to_wbuf(dartt_bl_t * pbl, const unsigned char * pointer);	//helper function for loading a pointer INTO the working buffer
uint32_t dartt_bl_load_wbuf_to_ptr(dartt_bl_t * pbl, unsigned char ** p_pointer);

static unsigned char * working_target_ptr_ = NULL;	//assigned using the working buffer on target architecture. 

/*Initialize the bootloader. calls unimplemented helper functions*/
void dartt_bl_init(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return;	//invalid pbl, silent failure
	}
	//initialize the filesystem
	if(dartt_bl_load_fds(pbl) != DARTT_BL_SUCCESS)
	{
		pbl->action_status = DARTT_BL_INITIALIZATION_FAILURE;
		return;
	}
	//initialize attributes
	if(dartt_bl_get_attributes(pbl) != DARTT_BL_SUCCESS)
	{
		pbl->action_status = DARTT_BL_INITIALIZATION_FAILURE;	//if you ever see this code, panic
		return;
	}
	working_target_ptr_ = NULL;	//soft reset won't load null, so do it in init. helps with test isolation too so state isn't mutated between tests
	pbl->action_status = DARTT_BL_INITIALIZED;
}


/*Main event handler. called in a loop.*/
void dartt_bl_event_handler(dartt_bl_t * pbl)
{
	dartt_bl_handle_comms(pbl);	//update pbl struct from comm interfaces

	if(pbl->action_flag != NO_ACTION)
	{
		switch (pbl->action_flag)
		{
			case START_APPLICATION:
			{
				dartt_bl_cleanup_system();
				dartt_bl_start_application(pbl);
				break;
			}
			case READ_BUFFER:
			{
				pbl->action_status = dartt_bl_read_mem(pbl);
				break;
			}
			case WRITE_BUFFER:
			{
				pbl->action_status = dartt_bl_flash_write(pbl);
				break;
			}
			case ERASE_PAGES:
			{
				pbl->action_status = dartt_bl_flash_erase(pbl);
				break;
			}
			case GET_CRC32:
			{
				pbl->action_status = dartt_bl_get_crc32(pbl);
				break;
			}
			case SAVE_SETTINGS:
			{
				pbl->action_status = dartt_bl_update_persistent_settings(pbl);
				break;
			}
			case GET_VERSION_HASH:
			{
				break;
			}
			case GET_APPLICATION_START_ADDR:
			{
				pbl->action_status = dartt_bl_load_ptr_to_wbuf(pbl, application_start_addr__);
				break;
			}
			case GET_WORKING_ADDR:
			{
				pbl->action_status = dartt_bl_load_ptr_to_wbuf(pbl, working_target_ptr_);
				break;
			}
			case SET_WORKING_ADDR:
			{
				pbl->action_status = dartt_bl_load_wbuf_to_ptr(pbl, &working_target_ptr_);
				break;
			}
			default:
			{
				pbl->action_status = DARTT_BL_INVALID_ACTION_REQUEST;
				break;
			}
		}
		pbl->action_flag = NO_ACTION;
	}	
}


/*
	Read requested target memory range into the working buffer.

	Assumes flash is always memory mapped. If your system runs from external
	flash memory, you will need to externally define this function with the appropriate
	read operations, or else drop in your own.

	Note - this function can, and likely will, result in hardfaults, frequently. The caller 
	should take care to check for initializatio status codes after calling this, erase, write, 
	or crc32 functions.
*/
uint32_t dartt_bl_read_mem(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(pbl->working_size > sizeof(pbl->working_buffer))
	{
		return DARTT_BL_WORKING_SIZE_INVALID;
	}
	if(pbl->working_size == 0)
	{
		return DARTT_BL_BAD_READ_REQUEST;
	}
	if(working_target_ptr_ == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	for(uint32_t i = 0; i < pbl->working_size; i++)
	{
		pbl->working_buffer[i] = working_target_ptr_[i];
	}
	return DARTT_BL_SUCCESS;
}

/*
	Loads the crc32 register for verification.
*/
uint32_t dartt_bl_get_crc32(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	
	const unsigned char * p_rmem = application_start_addr__;	//compile-time constant
	size_t app_size = (size_t)(pbl->fds.application_size);	//IMPORTANT NOTES: the fixed size of application_size means on a 64 bit system

	pbl->fds.application_crc32 = get_crc32(p_rmem, app_size);
	return DARTT_BL_SUCCESS;
}

/*
	Fulfill the action of loading the application start pointer into the working buffer. This is used for two purposes:
	1. Validity check for the flashing tool. If the blob we're trying to flash does not have a matching start address, 
		that's an exit-able error - it means the requested operation is impossible.
	2. This method also neatly doubles as a way to validate that the target system is 32bit.
		In reality we're basically always targeting 32-bit, but it's good to not design yourself into a corner.	
*/
uint32_t dartt_bl_load_ptr_to_wbuf(dartt_bl_t * pbl, const unsigned char * pointer)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(sizeof(unsigned char *) > sizeof(pbl->working_buffer))
	{
		return DARTT_BL_ERROR_POINTER_OVERRUN;	 //you never know? (lol)
	}

	pbl->working_size = 0;
	uintptr_t app_start = (uintptr_t)(pointer);
	for(int i = 0; i < sizeof(unsigned char *); i++)
	{	
		uintptr_t shift = i*8;
		pbl->working_buffer[i] = (app_start & (((uintptr_t)0xFF) << shift)) >> shift;
		pbl->working_size++;
	}
	return DARTT_BL_SUCCESS;
}


/*
Pass by reference load into a destination pointer
*/
uint32_t dartt_bl_load_wbuf_to_ptr(dartt_bl_t * pbl, unsigned char ** p_pointer)
{
	if(pbl == NULL || p_pointer == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(pbl->working_size != sizeof(unsigned char *))	//this is target/implementation specific - flashing tool should always read a pointer into the working buffer first to obtain the target pointer size.
	{
		return DARTT_BL_WORKING_SIZE_INVALID;
	}
	uintptr_t p = 0;
	for(int i = 0; i < sizeof(unsigned char *); i++)
	{
		int shift = i*8;
		p |= ((uintptr_t)(pbl->working_buffer[i])) << shift;
	}
	(*p_pointer) = (unsigned char *)p;
	return DARTT_BL_SUCCESS;
}
