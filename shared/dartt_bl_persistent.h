#ifndef DARTT_BL_PERSISTENT_H
#define DARTT_BL_PERSISTENT_H
#include <stdint.h>

/*
	TODO: reserve exactly one page at the top of the bootloader region, just under the application start, for the bootloader persistent settings
*/

/**
 * @brief Persistent bootloader settings stored in non-volatile memory.
 * @note Layout must remain stable across firmware versions. Any structural change requires a migration strategy.
 */
typedef struct dartt_bl_persistent_t
{
	uint32_t module_number;     /**< Primary DARTT node address for this device. */
	uint32_t application_size;  /**< Application image size in bytes, offset from @c application_start_addr__. */
	uint32_t application_crc32; /**< CRC32 of the application image. Used for startup validity check. */
	uint32_t boot_mode;         /**< If set to the magic word, bootloader jumps to application without waiting for @c START_APPLICATION. */
} dartt_bl_persistent_t;


#endif
