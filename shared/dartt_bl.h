#ifndef DARTT_BL_H
#define DARTT_BL_H
#include "dartt_bl_persistent.h"

enum {
	NO_ACTION,
	START_APPLICATION,	//action flag to start flashed application
	READ_BUFFER, 	//dispatch flag for reading the target flash memory region into the working buffer. Check for error codes after each op
	WRITE_BUFFER, 	//dispatch flag for writing the target flash memory region from the working buffer
	ERASE_PAGES,	//dispatch flag for erasing the target page(s)
	GET_CRC32,		//dispatch flag for get the CRC32 of the current flashed application
	SAVE_SETTINGS,
	GET_VERSION_HASH,	//dispatch flag to load the --short version hash in to the working buffer	
	GET_APPLICATION_START_ADDR,	//loads target-specific pointer into the working buffer
	GET_WORKING_ADDR,	//loads the current working address pointer into the working buffer
	SET_WORKING_ADDR	//loads contents of the current working buffer into the global, statically scoped working buffer pointer (target specific size)
};

enum {
	DARTT_BL_SUCCESS = 0,
	DARTT_BL_INITIALIZED = 1,	//used primarily for the flashing tool to track restarts. If this gets set to 1 after a potentially dangerous op, the flashing tool knows it did a bad thing that triggered a hardfault->reset
	DARTT_BL_INITIALIZATION_FAILURE = 2,	//indicate failure to initialize. Also used for flashing tool for unambiguous indication of a hardfault, separate branch for hardfault leading to catastrophic initialization error
	DARTT_BL_NULLPTR = -1,
	DARTT_BL_WORKING_SIZE_INVALID = -2,	//workign buffer read request size invalid
	DARTT_BL_APPLICATION_RANGE_INVALID = -3,
	DARTT_BL_INVALID_ACTION_REQUEST = -4,
	DARTT_BL_BAD_READ_REQUEST = -5,
	DARTT_BL_ERROR_POINTER_OVERRUN = -6,
	DARTT_BL_ERASE_FAILED_INVALID_PAGE_SIZE = -7,
	DARTT_BL_ERASE_FAILED_INVALID_NUMPAGES = -8,
	DARTT_BL_ERASE_BLOCKED = -9,
	DARTT_BL_WRITE_BLOCKED = -10
};

/*
Container for target attributes
*/
typedef struct dartt_bl_attributes_t
{
	//read-only section (read only not enforced as of )
	uint32_t page_size;	//number of bytes
	uint32_t write_size;
}dartt_bl_attributes_t;


/*
	Definition of the control structure.
	This is memory mapped to dartt and is the primary communication/control surface for the bootloader
*/
typedef struct dartt_bl_t
{
	uint32_t action_flag;	//when this is assigned, perform some enumerated action. Gets written to 0 when done for caller polling purposes.
	uint32_t action_status;	//dedicated word for return codes - where we check for errors, etc

	//uint32_t working_address;	//The start address of the data to read from or write to flash data storage, when triggered by an action flag
	uint32_t working_size;		//casted to a pointer. the offset from working_address
	unsigned char working_buffer[64];	//primary working buffer for the bootloader. Can be used for reads or writes
	
	//Erasure interface
	uint32_t erase_page;	//page index
	uint32_t erase_num_pages;	//number of pages starting from the page index to erase

	dartt_bl_attributes_t attr;	//attributes which must be loaded by the get attributes stub	
	dartt_bl_persistent_t fds;	//persistent settings for the bootloader, such as module number, a shared secret for decryption, etc. Currently only used for module number
}dartt_bl_t;

/*Initialize the bootloader. calls unimplemented helper functions*/
void dartt_bl_init(dartt_bl_t * pbl);

/*main event handler. */
void dartt_bl_event_handler(dartt_bl_t * pbl);

/*Helper function to load a pointer into the working buffer. Can be used by both flashing tool and bootloader*/
uint32_t dartt_bl_load_ptr_to_wbuf(dartt_bl_t * pbl, const unsigned char * pointer);	//helper function for loading a pointer INTO the working buffer

/*Helper function to working buffer into the load a pointer. Can be used by both flashing tool and bootloader*/
uint32_t dartt_bl_load_wbuf_to_ptr(dartt_bl_t * pbl, unsigned char ** p_pointer);

/*Helper function to retrive the page address based on the settings in the bootloader control. Useful for target implementer as well*/
uintptr_t dartt_bl_get_page_addr(dartt_bl_t * pbl);

#endif
