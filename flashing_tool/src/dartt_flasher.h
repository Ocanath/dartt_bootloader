#ifndef DARTT_FLASHER_H
#define DARTT_FLASHER_H

#include "dartt_sync.h"
#include "dartt_bl.h"
#include "serial.h"
#include "binary_file_handler.h"
#include <string.h>


class DarttFlasher
{
	public:
		DarttFlasher(unsigned char addr);
		~DarttFlasher();
		Serial ser;
		
		int init(void);

		int get_version(std::string & version);

		int write_file(BinaryFileHandler& handler, bool verify, bool skip_save, uintptr_t start_ptr=0);
		int readback_verification(BinaryFileHandler& handler, uintptr_t start_ptr=0);	//byte for byte read back verification
		int read_to_file(BinaryFileHandler& handler, uintptr_t start_ptr=0, size_t len=0);
		int get_file_crc(BinaryFileHandler& handler, uint32_t& crc);
		int verify_app(uint32_t crc32);

		uintptr_t get_app_start() const { return app_start; }
		uintptr_t get_flash_start() const { return flash_start; }

		int mass_erase(void);
		
		int start_app(void);

		uint32_t timeout;


		enum {
			FLASHER_SUCCESS = 0,
			ERROR_TIMEOUT = -100,
			ERROR_VERSION_RETRIEVAL_FAILED = -101,
			ERROR_PTR_RETRIEVAL_FAILED = -102,
			ERROR_INVALID_ARGUMENT = -103,
			ERROR_POINTERSIZE_NOT_LOADED = -104,
			ERROR_MEMORY_OVERRUN = -105,
			ERROR_POINTERSIZE_TOO_LARGE = -106,
			ERROR_ALREADY_INITIALIZED = -107,
			ERROR_NOT_INITIALIZED = -108,
			ERROR_NOTHING_TO_ERASE = -109,
			ERROR_ATTR_INVALID = -110,
			ERROR_LOAD_FAILED = -111,
			ERROR_VERIFY_FAILED = -112,
			ERROR_READ_FAILED = -114
		};

	private:
		bool initialized;

		unsigned char * tx_buf_mem;
		unsigned char * rx_buf_mem;
		
		size_t target_pointer_size;	//attribute of target
		dartt_bl_attributes_t attr_cpy;	//copy of base attributes
		uintptr_t app_start;
		uintptr_t flash_start;

		dartt_bl_t bootloader_control;
		dartt_bl_t bootloader_periph;

		dartt_sync_t ds;

		dartt_mem_t action_flags;
		dartt_mem_t working_buffer;
		dartt_mem_t erase_region;

		int get_target_pointer_size(void);

		int poll_action_flags(uint32_t timeout_ms);
		int write_action_flag(uint32_t flag);	//helper to load control with proper flags before dispatch. called before poll
		uintptr_t get_pointer(uint32_t flag);
		int set_working_pointer(uintptr_t pointer);
		int write_working_buffer(void);
		int read_working_buffer(void);
		int get_page_idx_of_pointer(uintptr_t pointer, uint32_t & page_idx);
		int erase_blob(uintptr_t start, size_t size);
};

#endif

