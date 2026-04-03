#ifndef DARTT_BL_PERSISTENT_H
#define DARTT_BL_PERSISTENT_H
#include <stdint.h>

/*
Debating whether this should go at the beginning of the persistent configuration page, or at the end,
or at the end with some pad for appending more persistent settings.
*/
typedef struct dartt_bl_persistent_t
{
	uint32_t module_number;	//primary dartt address stored here
	uint32_t application_size;		//size the application, in bytes. Restricts writeable size to ~2gb. Offset from the compile-time constant start address.
	uint32_t application_crc32;			//for checking validity of the application on startup
	uint32_t boot_mode;		//unless this is a magic word, the bootloader always awaits a START_APPLICATION action before branching to main program.	
}dartt_bl_persistent_t;


#endif