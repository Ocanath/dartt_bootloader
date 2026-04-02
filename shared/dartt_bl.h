#ifndef DARTT_BL_H
#define DARTT_BL_H
#include "dartt_bl_persistent.h"

enum {NO_ACTION, START_APPLICATION, READ_BUFFER, WRITE_BUFFER};

#define END_SIGNATURE_WORD1


//This typedef struct defines the blueprint/memory layout of the primary bootloader control structure
typedef struct dartt_bl_t
{
	uint32_t action_flag;	//when this is assigned, perform some enumerated action. Gets 
	

	uint32_t erase_page;
	uint32_t erase_num_pages;

	uint32_t working_address;	//The start address of the data to read from or write to flash data storage, when triggered by an action flag
	uint32_t working_size;		//casted to a pointer. the offset from working_address
	unsigned char working_buffer[64];	//primary working buffer for the bootloader. Can be used for reads or writes
	
	dartt_bl_persistent_t fds;	
	/* data */
}dartt_bl_t;


#endif