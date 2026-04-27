#ifndef DARTT_BL_H
#define DARTT_BL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dartt_bl_persistent.h"

#define DARTT_BL_START_PROGRAM_KEY	0xFE47BA32	//magic keyword. If boot_mode is equal to this, you auto start unless the ram key is also equal to this - then you await a start instruction

enum {
	NO_ACTION = 0,
	START_APPLICATION = 1,	//action flag to start flashed application
	READ_BUFFER = 2, 	//dispatch flag for reading the target flash memory region into the working buffer. Check for error codes after each op
	WRITE_BUFFER = 3, 	//dispatch flag for writing the target flash memory region from the working buffer
	ERASE_PAGES = 4,	//dispatch flag for erasing the target page(s)
	GET_CRC32 = 5,		//dispatch flag for get the CRC32 of the current flashed application
	SAVE_SETTINGS = 6,	//dispatch flag to write pbl->fds to the persistent settings flash page. Invalidates working_size and working_buffer on return
	GET_VERSION_HASH = 7,	//dispatch flag to load the --short version hash in to the working buffer
	GET_APPLICATION_START_ADDR = 8,	//loads target-specific pointer into the working buffer
	GET_WORKING_ADDR = 9,	//loads the current working address pointer into the working buffer
	SET_WORKING_ADDR = 10,	//loads contents of the current working buffer into the global, statically scoped working buffer pointer (target specific size)
	GET_FLASH_BASE_ADDR = 11	//loads the current flash base pointer into the working buffer
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
	DARTT_BL_WRITE_SIZE_UNINITALIZED = -11,
	DARTT_BL_GIT_HASH_LOAD_OVERRUN = -12,
	DARTT_BL_GIT_HASH_NOT_FOUND = -14,
	DARTT_BL_OUT_OF_BOUNDS = -15,
	DARTT_BL_FDS_LOAD_FAILED = -16
};

/**
 * @brief Target flash attributes. Populated by @c dartt_bl_get_attributes() on init.
 */
typedef struct dartt_bl_attributes_t
{
	uint32_t page_size;  /**< Erasable page size in bytes. */
	uint32_t num_pages;	/**<  Number of pages on target */
	uint32_t write_size; /**< Minimum flash write granularity in bytes. Must be a power of 2. */
} dartt_bl_attributes_t;


/**
 * @brief Primary bootloader control structure. Memory-mapped as the DARTT register interface.
 */
typedef struct dartt_bl_t
{
	uint32_t action_flag;             /**< Write an action enum to dispatch a deferred operation. Cleared to @c NO_ACTION on completion. */
	uint32_t action_status;           /**< Return code of the most recently completed action. Valid only when @c action_flag is @c NO_ACTION. */

	uint32_t working_size;            /**< Byte count for read/write operations. Must be a nonzero multiple of @c attr.write_size for writes. */
	unsigned char working_buffer[64]; /**< Data buffer for read/write operations. */

	uint32_t erase_page;              /**< Absolute page index (from @c flash_base_addr__) to erase. */
	uint32_t erase_num_pages;         /**< Number of pages to erase starting from @c erase_page. */

	dartt_bl_attributes_t attr;       /**< Target flash attributes. Populated by @c dartt_bl_get_attributes(). Read-only at runtime. */
	dartt_bl_persistent_t fds;        /**< Persistent bootloader settings. Loaded from NVM on init. */
} dartt_bl_t;

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
 * @return Byte address of @c pbl->erase_page, or @c NULL if @c pbl is NULL.
 */
uintptr_t dartt_bl_get_page_addr(dartt_bl_t * pbl);

/**
 * @brief Compute the byte address of the last address of memory. Used for out of bounds checks
 * @param pbl Pointer to bootloader control structure.
 * @return Byte address of @c pbl->erase_page, or @c NULL if @c pbl is NULL.
 */
uintptr_t dartt_bl_get_flash_end(dartt_bl_t * pbl);

#ifdef __cplusplus
}
#endif

#endif
