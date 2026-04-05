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
	DARTT_BL_WRITE_BLOCKED = -10,
	DARTT_BL_WRITE_SIZE_UNINITALIZED = -11
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
	//deferred action registers
	uint32_t action_flag;	//when this is assigned, perform some enumerated action. Gets written to 0 when done for caller polling purposes.
	uint32_t action_status;	//dedicated word for return codes - where we check for errors, etc

	//working buffer
	uint32_t working_size;		//casted to a pointer. the offset from working_address
	unsigned char working_buffer[64];	//primary working buffer for the bootloader. Can be used for reads or writes
	
	//Erasure interface
	uint32_t erase_page;	//page index
	uint32_t erase_num_pages;	//number of pages starting from the page index to erase

	//attributes and persistent settings
	dartt_bl_attributes_t attr;	//attributes which must be loaded by the get attributes stub	
	dartt_bl_persistent_t fds;	//persistent settings for the bootloader, such as module number, a shared secret for decryption, etc. Currently only used for module number
}dartt_bl_t;

/**
 * @brief Initialize the bootloader. Loads persistent settings and target attributes.
 * @param pbl Pointer to bootloader control structure.
 * @note Sets @c action_status to @c DARTT_BL_INITIALIZED on success, @c DARTT_BL_INITIALIZATION_FAILURE on error.
 */
void dartt_bl_init(dartt_bl_t * pbl);

/**
 * @brief Main event handler. Call in a loop. Processes incoming comms and dispatches deferred actions.
 * @param pbl Pointer to bootloader control structure.
 */
void dartt_bl_event_handler(dartt_bl_t * pbl);

/**
 * @brief Serialize a pointer address into the working buffer (little-endian). Sets @c working_size to pointer width.
 * @param pbl     Pointer to bootloader control structure.
 * @param pointer Address to serialize.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_load_ptr_to_wbuf(dartt_bl_t * pbl, const unsigned char * pointer);

/**
 * @brief Deserialize a pointer address from the working buffer (little-endian).
 * @param pbl       Pointer to bootloader control structure.
 * @param p_pointer Output pointer. Set to the deserialized address on success.
 * @return @c DARTT_BL_SUCCESS or error code. Fails if @c working_size does not equal pointer width.
 */
uint32_t dartt_bl_load_wbuf_to_ptr(dartt_bl_t * pbl, unsigned char ** p_pointer);

/**
 * @brief Returns the current working target pointer. For use by flash write stub implementations.
 * @return Current value of @c working_target_ptr_.
 */
unsigned char * dartt_bl_get_working_ptr(void);

/**
 * @brief Compute the byte address of the requested erase page.
 * @param pbl Pointer to bootloader control structure.
 * @return Byte address of @c pbl->erase_page, or @c DARTT_BL_NULLPTR if @c pbl is NULL.
 */
uintptr_t dartt_bl_get_page_addr(dartt_bl_t * pbl);

#endif
