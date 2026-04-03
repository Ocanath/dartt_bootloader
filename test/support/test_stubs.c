#include "dartt_bl_stubs.h"

unsigned char fake_application_area[0x2000] = {0xFFFFFFFF};	

uint32_t dartt_bl_load_fds(dartt_bl_t * pbl)
{
	if(pbl == NULL)
	{
		return DARTT_BL_NULLPTR;
	}
	pbl->fds.application_end_addr = (uint32_t)(&fake_application_area[sizeof(fake_application_area) - 1]);
	pbl->application_start_address = (uint32_t)(&fake_application_area[0]); 
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


