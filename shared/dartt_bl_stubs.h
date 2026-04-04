#ifndef DARTT_BL_STUBS_H
#define DARTT_BL_STUBS_H

#include "dartt_bl.h"

/*Implementation-specific stubs. All of these functions must have an application defined implementation for the bootloader to function*/

extern const unsigned char * application_start_addr__;	//must be defined by caller, a compile time constant

/*
This must load the filesystem from persistent storage.
The filesystem gets loaded into pbl->fds.
The filesystem must have the correct structure.
*/
uint32_t dartt_bl_load_fds(dartt_bl_t * pbl);

/*
This must load everything in attr.

The bootloader checks for non-default values (nonzero) to make sure they're set
*/
uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl);


/*write the persistent config memory region*/
uint32_t dartt_bl_update_persistent_settings(dartt_bl_t * pbl);

/*
	This must handle any/all comm interfaces that are in use to pbl.
	MUST make a call to:
		1. dartt_frame_to_payload
		2. on success, dartt_parse_general_message
	
	In addition to any framing/decoding logic, such as DMA polling, COBS unstuffing, decryption, etc.
	
	All interfaces should be called in this stub. Called within the event handler.


	Important note: this puts the burden of whether the layout is standards compliant on the embedded system/implementation.
	You could pass raw structs to dartt, or work with raw buffers and add an indirection layer for mapping it to the control struct symbols.
	In reality on an embedded system, there's no reason to not use raw structs if the MCU implementation is:
		1. little endian
		2. 32bit 
*/
uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl);


/*
	Stub to reset all clock and peripheral settings
*/
uint32_t dartt_bl_cleanup_system(void);	//stub called in START_APPLICATION

/*
	Set VTOR, load MSP, jump to application code
*/
uint32_t dartt_bl_start_application(dartt_bl_t * pbl);

/*
	Flash write. Implement for target
*/
uint32_t dartt_bl_flash_write(dartt_bl_t * pbl);

/*
	Flash erase. Implement for target
*/
uint32_t dartt_bl_flash_erase(dartt_bl_t * pbl);

#endif
