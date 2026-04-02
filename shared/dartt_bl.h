#ifndef DARTT_BL_H
#define DARTT_BL_H
#include "dartt_bl_persistent.h"

enum {NO_ACTION,
	START_APPLICATION,	//action flag to start flashed application
	READ_BUFFER, 	//dispatch flag for reading the target flash memory region into the working buffer. Check for error codes after each op
	WRITE_BUFFER, 	//dispatch flag for writing the target flash memory region from the working buffer
	ERASE_PAGES,	//dispatch flag for erasing the target page(s)
	GET_CRC32,		//dispatch flag for get the CRC32 of the current flashed application
	SAVE_SETTINGS,
	GET_VERSION_HASH	//dispatch flag to load the --short version hash in to the working buffer	
};


//This typedef struct defines the blueprint/memory layout of the primary bootloader control structure
typedef struct dartt_bl_t
{
	uint32_t action_flag;	//when this is assigned, perform some enumerated action. Gets written to 0 when done for caller polling purposes.
	
	uint32_t action_status;	//dedicated word for return codes - where we check for errors, etc

	//the bootloader will have to protect itself from erasure using some mechanism - either find its size on startup with some process or have it compiled in via scripting
	uint32_t erase_page;
	uint32_t erase_num_pages;

	uint32_t working_address;	//The start address of the data to read from or write to flash data storage, when triggered by an action flag
	uint32_t working_size;		//casted to a pointer. the offset from working_address
	unsigned char working_buffer[64];	//primary working buffer for the bootloader. Can be used for reads or writes

	uint32_t crc32;	//result of the latest triggered GET_CRC32 operation
	
	//read-only section (read only not enforced as of )
	uint32_t page_size;	//number of bytes
	uint32_t bootloader_end__;	//a compile time constant, set by the linker. The start of the application, and the end of the bootloader.

	dartt_bl_persistent_t fds;	//persistent settings for the bootloader, such as module number, a shared secret for decryption, etc. Currently only used for module number
}dartt_bl_t;


#endif