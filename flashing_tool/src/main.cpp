#include <stdio.h>
#include "serial.h"
#include "dartt_sync.h"
#include "args.h"
#include "dartt_bl.h"
#include "callbacks.h"
#include "version.h"
#include "dartt_flasher.h"
#include "binary_file_handler.h"

int main(int argc, char** argv)
{
	args_t args = parse_args(argc, argv);

	DarttFlasher flasher(args.dartt_address);
	flasher.ser.autoconnect(args.baudrate);
	flasher.init();
	uint32_t crc32 = 0;	//for verification

	if(args.get_version)
	{
		printf("Flashing Tool Version: %s\n", firmware_version);

		std::string version;
		int rc = flasher.get_version(version);	
		if(rc == 0)
		{
			printf("Bootloader Device Version: ");
			printf("%s\n",version.c_str());
			return rc;
		}
		else
		{
			printf("Error: code %d\n", rc);
			return rc;
		}
	}
	else if(args.eraseall)
	{
		int rc = flasher.mass_erase();
		if(rc == 0)
		{
			printf("Mass erase success\n");
		}
		else
		{
			printf("Error: code %d\n", rc);
		}
		return rc;	//don't continue to do anything else if masserase is called
	}
	else if(args.launch)
	{
		printf("Sending start signal!\n");
		return flasher.start_app();
	}
	if(args.output_file != NULL)
	{
		uintptr_t rorigin = args.has_rorigin ? args.rorigin : 0;
		size_t rlen = args.has_rlen ? args.rlen : 0;
		BinaryFileHandler out_handler(args.output_file, true);
		if(!out_handler.is_valid())
		{
			printf("Error: could not open output file %s\n", args.output_file);
			return 1;
		}
		int rc = flasher.read_to_file(out_handler, rorigin, rlen);
		if(rc == DarttFlasher::FLASHER_SUCCESS)
		{
			printf("Read complete\n");
		}
		else
		{
			printf("Error: %d\n", rc);
		}
		return rc;
	}

	if(args.filename == NULL)
	{
		printf("Error: no file specified\n");
		return 1;
	}

	BinaryFileHandler handler(args.filename);
	if(!handler.is_valid())
	{
		printf("Error: could not open file %s\n", args.filename);
		return 1;
	}
	if(handler.get_file_type() == FileType::ELF && !args.no_application)
	{
		uintptr_t elf_addr = handler.get_block_address();
		if(elf_addr != flasher.get_app_start())
		{
			printf("Error: file start address does not match the target start address 0x%lX. Did you mean to use --no-application?\n",
				(unsigned long)flasher.get_app_start());
			return 1;
		}
	}

	if(args.verify_only)
	{
		bool use_readback = args.has_origin_addr || args.no_application;
		if(!use_readback)
		{
			int rc = flasher.get_file_crc(handler, crc32);
			if(rc != DarttFlasher::FLASHER_SUCCESS)
			{
				printf("Error: unable to get crc from file\n");
				return rc;
			}
			rc = flasher.verify_app(crc32);
			if(rc == DarttFlasher::FLASHER_SUCCESS)
			{
				printf("CRC Verification success!\n");
			}
			else
			{
				printf("Error: CRC Verification. Code %d\n", rc);
			}
			return rc;
		}
		else
		{
			uintptr_t vaddr = args.has_origin_addr ? args.origin_addr : handler.get_block_address();
			int rc = flasher.readback_verification(handler, vaddr);
			if(rc == DarttFlasher::FLASHER_SUCCESS)
			{
				printf("Readback Verification Success!\n");
			}
			else
			{
				printf("Error: Readback Verification. Code %d\n", rc);
			}
			return rc;
		}
	}

	uintptr_t start = 0;
	if(args.has_origin_addr)
	{
		start = args.origin_addr;
	}
	else if(handler.get_file_type() == FileType::ELF)
	{
		start = handler.get_block_address();
	}
	int rc = flasher.write_file(handler, !args.no_verify, args.skip_save, start);
	if(rc == DarttFlasher::ERROR_VERIFY_FAILED)
	{
		printf("Error: Verification Failed!\n");
	}
	else if(rc != DarttFlasher::FLASHER_SUCCESS)
	{
		printf("Error %d\n", rc);
	}

	return rc;
}

