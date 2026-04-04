#include "dartt_bl_stubs.h"
#include "unity.h"

unsigned char fake_application_area[0x2000] = {0xFFFFFFFF};	
const unsigned char * application_start_addr__ = fake_application_area;	//must define this

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


uint32_t dartt_bl_flash_erase(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

