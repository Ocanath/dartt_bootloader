#include "dartt_bl.h"
#include "dartt_bl_stubs.h"
#include "checksum.h"
#include <stdbool.h>
#include "version.h"

uint32_t dartt_bl_read_mem(dartt_bl_t * pbl);
uint32_t dartt_bl_get_crc32(dartt_bl_t * pbl);
uint32_t dartt_bl_load_ptr_to_wbuf(dartt_bl_t * pbl, const unsigned char * pointer);	//helper function for loading a pointer INTO the working buffer
uint32_t dartt_bl_load_wbuf_to_ptr(dartt_bl_t * pbl, unsigned char ** p_pointer);
uint32_t dartt_bl_check_erase_request(dartt_bl_t * pbl);
uint32_t dartt_bl_check_write_request(dartt_bl_t * pbl);
uint32_t dartt_bl_load_git_hash(dartt_bl_t * pbl);

static unsigned char * working_target_ptr_ = NULL;	//assigned using the working buffer on target architecture. 

/** @brief Initialize the bootloader. See dartt_bl.h for full documentation. */
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
	if(pbl->attr.page_size == 0 || 
		pbl->attr.write_size == 0 ||
		pbl->attr.num_pages == 0
	)
	{
		pbl->action_status = DARTT_BL_INITIALIZATION_FAILURE;
		return;
	}
	working_target_ptr_ = NULL;	//soft reset won't load null, so do it in init. helps with test isolation too so state isn't mutated between tests
	pbl->action_status = DARTT_BL_INITIALIZED;
}


/** @brief Main event handler. See dartt_bl.h for full documentation. */
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
				pbl->action_status = dartt_bl_check_write_request(pbl);
				if(pbl->action_status == DARTT_BL_SUCCESS)
				{
					pbl->action_status = dartt_bl_flash_write(pbl);
				}
				break;
			}
			case ERASE_PAGES:
			{
				pbl->action_status = dartt_bl_check_erase_request(pbl);
				if(pbl->action_status == DARTT_BL_SUCCESS)
				{
					pbl->action_status = dartt_bl_flash_erase(pbl);
				}
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
				pbl->action_status = dartt_bl_load_git_hash(pbl);
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


/**
 * @brief Read @c working_size bytes from @c working_target_ptr_ into @c working_buffer.
 * @note Assumes memory-mapped flash. May trigger a hardfault on invalid addresses — caller
 *       should check @c action_status for @c DARTT_BL_INITIALIZED after any potentially unsafe op.
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

/**
 * @brief Compute CRC32 of the application image and store in @c pbl->fds.application_crc32.
 * @note Uses @c application_start_addr__ and @c pbl->fds.application_size as the range.
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

/** @brief Serialize a pointer into the working buffer. See dartt_bl.h for full documentation. */
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


/** @brief Deserialize a pointer from the working buffer. See dartt_bl.h for full documentation. */
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

unsigned char * dartt_bl_get_working_ptr(void)
{
	return working_target_ptr_;
}

uintptr_t dartt_bl_get_page_addr(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	return ((uintptr_t)flash_base_addr__) + ((uintptr_t)pbl->attr.page_size) * ((uintptr_t)pbl->erase_page);
}

uint32_t dartt_bl_check_write_request(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(working_target_ptr_ == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(pbl->attr.write_size == 0)	//redundant with init checks but good to validate anyway
	{
		return DARTT_BL_WRITE_SIZE_UNINITALIZED;
	}
	if(pbl->working_size == 0 || 
		pbl->working_size > sizeof(pbl->working_buffer) ||
		pbl->working_size % pbl->attr.write_size != 0)
	{
		return DARTT_BL_WORKING_SIZE_INVALID;
	}
	if(working_target_ptr_ < application_start_addr__)
	{
		return DARTT_BL_WRITE_BLOCKED;
	}
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_check_erase_request(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(pbl->attr.page_size == 0)
	{
		return DARTT_BL_ERASE_FAILED_INVALID_PAGE_SIZE;
	}
	if(pbl->erase_num_pages == 0)
	{
		return DARTT_BL_ERASE_FAILED_INVALID_NUMPAGES;
	}
	uintptr_t erase_addr = dartt_bl_get_page_addr(pbl);
	uintptr_t app_start = (uintptr_t)(application_start_addr__);
	if(erase_addr < app_start)
	{
		return DARTT_BL_ERASE_BLOCKED;
	}
	return DARTT_BL_SUCCESS;
}


uint32_t dartt_bl_load_git_hash(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	if(sizeof(firmware_version) > sizeof(pbl->working_buffer))
	{
		return DARTT_BL_GIT_HASH_LOAD_OVERRUN;
	}
	pbl->working_size = 0;	
	for(size_t i = 0; i < sizeof(firmware_version); i++)	
	{
		pbl->working_buffer[i] = firmware_version[i];
		pbl->working_size++;
	}
	if(sizeof(firmware_version) == 0)	//if not present, throw this code and set output size to zero. zero size = noop on working buffer copy
	{
		return DARTT_BL_GIT_HASH_NOT_FOUND;
	}
	return DARTT_BL_SUCCESS;
}
