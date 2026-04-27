#include "dartt_bl_stubs.h"
#include "dartt_bl_linker.h"
#include <stdbool.h>
#include "unity.h"

#define TEST_PAGE_SIZE 0x100

unsigned char fake_application_area[0x2000] = {};
uint32_t gl_magic_ram_word = 0xFFFFFFFF;
bool gl_start_called = false;
bool gl_cleanup_called = false;

const unsigned char * dartt_bl_get_flash_base(void) { return &fake_application_area[0]; }
const unsigned char * dartt_bl_get_app_start(void)  { return &fake_application_area[0x400]; }
const uint32_t dartt_bl_get_ram_blockstart_word(void)  { return (const uint32_t)(gl_magic_ram_word); }

const unsigned char flash_base_addr__[] = "if you are reading this, you fucked up the test environment";
const unsigned char application_start_addr__[] = "if you are reading this, you fucked up the test environment";
const unsigned char ram_blockstart_keyword_addr__[] = "if you are reading this, you fucked up the test environment";



/*
Test scoped helper
*/
void set_application_size(dartt_bl_t * pbl)
{
	uintptr_t flash_base = (uintptr_t)dartt_bl_get_flash_base();
	uintptr_t app_start = (uintptr_t)dartt_bl_get_app_start();
	uintptr_t flash_end = flash_base + (uintptr_t)sizeof(fake_application_area);
	pbl->fds.application_size = flash_end - app_start;
}

uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl)
{
	pbl->attr.page_size = TEST_PAGE_SIZE;
	pbl->attr.num_pages = sizeof(fake_application_area)/((size_t)pbl->attr.page_size);
	pbl->attr.write_size = 8;
	return DARTT_BL_SUCCESS;
}


uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl)
{
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_cleanup_system(void)	//stub called in START_APPLICATION
{
	gl_cleanup_called = true;
	return DARTT_BL_SUCCESS;
}

uint32_t dartt_bl_start_application(dartt_bl_t * pbl)
{
	gl_start_called = true;	//clean up when done
	return DARTT_BL_SUCCESS;
}


/*emulate write*/
uint32_t dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size)
{
	for(size_t i = 0; i < size; i++)
	{
		dest[i] = src[i];
	}
	return DARTT_BL_SUCCESS;
}


/*emulate erasure*/
uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages)
{
	uintptr_t erase_ptr = ((uintptr_t)dartt_bl_get_flash_base()) + ((uintptr_t)TEST_PAGE_SIZE) * ((uintptr_t)erase_page);
	size_t erase_range = ((size_t)erase_num_pages)*((size_t)TEST_PAGE_SIZE);
	for(size_t i = 0; i < erase_range; i++)
	{
		((unsigned char *)erase_ptr)[i] = 0xFF;
	}
	return DARTT_BL_SUCCESS;
}

