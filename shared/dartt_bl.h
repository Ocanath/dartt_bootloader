#ifndef DARTT_BL_H
#define DARTT_BL_H
#include "dartt_bl_persistent.h"

enum {NO_ACTION, START_APPLICATION, READ_BUFFER, WRITE_BUFFER, ERASE_PAGES, GET_CRC32};


//This typedef struct defines the blueprint/memory layout of the primary bootloader control structure
typedef struct dartt_bl_t
{
	uint32_t action_flag;	//when this is assigned, perform some enumerated action. Gets written to 0 when done for caller polling purposes.
	
	//the bootloader will have to protect itself from erasure using some mechanism - either find its size on startup with some process or have it compiled in via scripting
	uint32_t erase_page;
	uint32_t erase_num_pages;

	uint32_t working_address;	//The start address of the data to read from or write to flash data storage, when triggered by an action flag
	uint32_t working_size;		//casted to a pointer. the offset from working_address
	unsigned char working_buffer[64];	//primary working buffer for the bootloader. Can be used for reads or writes

	uint32_t target_start;	//start address of the firmware, for application start purposes and CRC32 calculation purposes
	uint32_t target_end;	//last application address, mainly for crc calculation purposes

	uint32_t crc32;	//result of the latest triggered GET_CRC32 operation
	
	dartt_bl_persistent_t fds;	
	/* data */
}dartt_bl_t;


#endif