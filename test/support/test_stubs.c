#include "dartt_bl_stubs.h"
#include "unity.h"

unsigned char fake_application_area[0x2000] = {};	
const unsigned char * application_start_addr__ = &fake_application_area[0x400];	//must define this
const unsigned char * flash_base_addr__ = &fake_application_area[0];

uint32_t dartt_bl_load_fds(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	TEST_ASSERT_LESS_THAN(0xFFFFFFFF, sizeof(fake_application_area));
	pbl->fds.application_size = sizeof(fake_application_area);
	
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
	pbl->attr.page_size = 0x100;
	pbl->attr.write_size = 8;
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_update_persistent_settings(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_cleanup_system(void)	//stub called in START_APPLICATION
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_start_application(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}


uint32_t dartt_bl_flash_write(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}


/*emulate erasure*/
uint32_t dartt_bl_flash_erase(dartt_bl_t * pbl)
{
	unsigned char * erase_ptr = (unsigned char *)dartt_bl_get_page_addr(pbl);
	size_t erase_range = ((size_t)pbl->erase_num_pages)*((size_t)pbl->attr.page_size);
	for(size_t i = 0; i < erase_range; i++)
	{
		erase_ptr[i] = 0xFF;
	}
	return DARTT_BL_SUCCESS;
}

