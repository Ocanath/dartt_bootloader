#ifndef DARTT_BL_STUBS_H
#define DARTT_BL_STUBS_H

#include "dartt_bl.h"

/*Implementation-specific stubs. All of these functions must have an application defined implementation for the bootloader to function*/

/*
This must load the filesystem from persistent storage.
The filesystem gets loaded into pbl->fds.
The filesystem must have the correct structure.
*/
uint32_t dartt_bl_load_fds(dartt_bl_t * pbl);

/*
This must load:
	page_size
	bootloader_end_address
The bootloader checks for non-default values (nonzero) to make sure they're set
*/
uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl);


/*write the persistent config memory region*/
uint32_t dartt_bl_update_filesystem(dartt_bl_t * pbl);

/*
	This must handle any/all comm interfaces that are in use to pbl.
	MUST make a call to:
		1. dartt_frame_to_payload
		2. on success, dartt_parse_general_message
	
	In addition to any framing/decoding logic, such as DMA polling, COBS unstuffing, decryption, etc.
	
	All interfaces should be called in this stub. Called within the event handler.
*/
uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl);


/*
	Read requested filesystem chunk into the working buffer.
*/
uint32_t dartt_bl_read_from_filesystem(dartt_bl_t * pbl);



#endif
