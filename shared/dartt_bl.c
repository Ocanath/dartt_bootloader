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
uint32_t dartt_bl_update_persistent_settings(dartt_bl_t * pbl);
uint32_t dartt_bl_load_fds(dartt_bl_t * pbl);

static unsigned char * working_target_ptr_ = NULL;	//assigned using the working buffer on target architecture. 

/** @brief Initialize the bootloader. See dartt_bl.h for full documentation. */
void dartt_bl_init(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return;	//invalid pbl, silent failure
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
	//initialize the filesystem. Must load attributes first to know where to find it
	if(dartt_bl_load_fds(pbl) != DARTT_BL_SUCCESS)
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
					pbl->action_status = dartt_bl_flash_write(working_target_ptr_, pbl->working_buffer, pbl->working_size);
				}
				break;
			}
			case ERASE_PAGES:
			{
				pbl->action_status = dartt_bl_check_erase_request(pbl);
				if(pbl->action_status == DARTT_BL_SUCCESS)
				{
					pbl->action_status = dartt_bl_flash_erase(pbl->erase_page, pbl->erase_num_pages);
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
 * @return @c DARTT_BL_OUT_OF_BOUNDS if read range exceeds flash region.
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
	uintptr_t flash_end = dartt_bl_get_flash_end(pbl);
	if((uintptr_t)working_target_ptr_ + (uintptr_t)pbl->working_size > flash_end)
	{
		return DARTT_BL_OUT_OF_BOUNDS;
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
 * @return @c DARTT_BL_OUT_OF_BOUNDS if the application image range exceeds flash region.
 */
uint32_t dartt_bl_get_crc32(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}

	const unsigned char * p_rmem = application_start_addr__;
	size_t app_size = (size_t)(pbl->fds.application_size);
	uintptr_t flash_end = (uintptr_t)flash_base_addr__ + (uintptr_t)pbl->attr.num_pages * (uintptr_t)pbl->attr.page_size;
	if((uintptr_t)application_start_addr__ + (uintptr_t)app_size > flash_end)
	{
		return DARTT_BL_OUT_OF_BOUNDS;
	}

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

uintptr_t dartt_bl_get_flash_end(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return 0;	//error case should return null for safety
	}
	return (uintptr_t)(flash_base_addr__) + (uintptr_t)pbl->attr.num_pages * (uintptr_t)pbl->attr.page_size;

}

uintptr_t dartt_bl_get_page_addr(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return 0;	//error case should return null for safety
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
	uintptr_t flash_end = dartt_bl_get_flash_end(pbl);
	if((uintptr_t)working_target_ptr_ + (uintptr_t)pbl->working_size > flash_end)
	{
		return DARTT_BL_OUT_OF_BOUNDS;
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
	if(pbl->erase_page + pbl->erase_num_pages > pbl->attr.num_pages)
	{
		return DARTT_BL_OUT_OF_BOUNDS;
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


/**
 * @brief Get a pointer to the start of the filesystem
 */
uintptr_t dartt_bl_get_fds_start_ptr(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return 0;
	}
	if(pbl->attr.page_size==0)
	{
		return 0;
	}
	uintptr_t app_start = (uintptr_t)application_start_addr__;
	uintptr_t page_size = (uintptr_t)pbl->attr.page_size;
	if(page_size > app_start)
	{
		return 0;	//underflow guard, only possible if the app start address is not properly defined (i.e. null, etc)
	}
	return app_start - page_size;
}

/**
 * @brief Get a pointer to the start of the filesystem
 * @param pbl Pointer to the bootloader control structure
 * @return Page index of the filesystem
 */
uint32_t dartt_bl_get_fds_start_page(dartt_bl_t * pbl)
{
	uintptr_t fds_ptr = dartt_bl_get_fds_start_ptr(pbl);
	if(fds_ptr != 0)
	{
		return (size_t)(fds_ptr - (uintptr_t)flash_base_addr__)/(size_t)pbl->attr.page_size;	//page size is checked by start so safe
	}
	return 0;	//return invalid page index if something is wrong
}

/**
 * @brief Load persistent settings from non-volatile storage into @c pbl->fds. Nonvolatile storage is always one page, reserved at the end of the bootloader section.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 */

uint32_t dartt_bl_load_fds(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}

	uintptr_t p_fds = dartt_bl_get_fds_start_ptr(pbl);
	if(p_fds == 0)
	{
		return DARTT_BL_FDS_LOAD_FAILED;
	}
	if(p_fds + (uintptr_t)(sizeof(dartt_bl_persistent_t)) >= application_start_addr__)
	{
		return DARTT_BL_FDS_LOAD_FAILED;
	}

	unsigned char * p_settings = (unsigned char *)(&pbl->fds);
	for(size_t i = 0; i < sizeof(dartt_bl_persistent_t); i++)
	{
		p_settings[i] = ((unsigned char *)p_fds)[i];
	}

	return DARTT_BL_SUCCESS;
}



/**
 * @brief Erase the persistent settings page and write @c pbl->fds to it.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 * @note Always sets @c working_size to zero on entry, invalidating working buffer contents.
 *       Uses @c working_buffer as scratch if @c sizeof(dartt_bl_persistent_t) is not a
 *       multiple of @c attr.write_size. Caller must reload working state after this call.
 */
uint32_t dartt_bl_update_persistent_settings(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	pbl->working_size = 0;	//always invalidate working buffer on entry — see dartt_bl.h contract note
	if(pbl->attr.write_size == 0)	//div by zero guard
	{
		return DARTT_BL_WRITE_SIZE_UNINITALIZED;
	}

	uint32_t fds_page = dartt_bl_get_fds_start_page(pbl);
	uint32_t rc = dartt_bl_flash_erase(fds_page, 1);
	if(rc != DARTT_BL_SUCCESS)
	{
		return rc;
	}
	size_t num_writes = sizeof(dartt_bl_persistent_t)/((size_t)pbl->attr.write_size);
	size_t nbytes_remainder = sizeof(dartt_bl_persistent_t) % ((size_t)pbl->attr.write_size);
	unsigned char * fds_dest = (unsigned char *)dartt_bl_get_fds_start_ptr(pbl);
	unsigned char * fds_src = (unsigned char *)(&pbl->fds);
	for(size_t i = 0; i < num_writes; i++)
	{
		rc = dartt_bl_flash_write(fds_dest, fds_src, pbl->attr.write_size);
		if(rc != DARTT_BL_SUCCESS)
		{
			return rc;
		}
		fds_dest += pbl->attr.write_size;
		fds_src += pbl->attr.write_size;
	}
	if(nbytes_remainder != 0)
	{
		for(size_t i = 0; i < pbl->attr.write_size; i++)
		{
			if(i < nbytes_remainder)
			{
				pbl->working_buffer[i] = fds_src[i];
			}
			else
			{
				pbl->working_buffer[i] = 0;
			}
		}
		rc = dartt_bl_flash_write(fds_dest, pbl->working_buffer, pbl->attr.write_size);
	}
	return rc;
}