#include "dartt_bl_linker.h"

extern const unsigned char flash_base_addr__[];
extern const unsigned char application_start_addr__[];

__attribute__((weak)) const unsigned char * dartt_bl_get_flash_base(void)
{
	return flash_base_addr__;
}

__attribute__((weak)) const unsigned char * dartt_bl_get_app_start(void)
{
	return application_start_addr__;
}
