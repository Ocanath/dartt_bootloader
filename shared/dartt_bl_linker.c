#include "dartt_bl_linker.h"
#include <stddef.h>

extern const unsigned char flash_base_addr__[];
extern const unsigned char application_start_addr__[];
extern const unsigned char ram_blockstart_keyword_addr__[];

__attribute__((weak)) const unsigned char * dartt_bl_get_flash_base(void)
{
	return flash_base_addr__;
}

__attribute__((weak)) const unsigned char * dartt_bl_get_app_start(void)
{
	return application_start_addr__;
}

__attribute__((weak)) const uint32_t dartt_bl_get_ram_blockstart_word(void)
{
	unsigned char * bufaddr = (unsigned char *)ram_blockstart_keyword_addr__;
	uint32_t val = 0;
	for(size_t i = 0; i < sizeof(val); i++)
	{
		val |= ( ((uint32_t)bufaddr[i]) << (i*8));
	}
	return val;
}
