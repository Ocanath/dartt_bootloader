/*
 * dartt_bl_stubs.c
 *
 *  Created on: Apr 7, 2026
 *      Author: ocanath
 */
#include "dartt_bl_stubs.h"


const unsigned char * flash_base_addr__ = (const unsigned char *)(0x08000000);	//TODO: DELETE THIS, GENERATE VIA LINKER
const unsigned char * application_start_addr__ = (const unsigned char *)(0x08003000);	//TODO: DELETE THIS, GENERATE VIA LINKER

uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_cleanup_system(void)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_start_application(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages)
{
	return DARTT_BL_SUCCESS;
}
