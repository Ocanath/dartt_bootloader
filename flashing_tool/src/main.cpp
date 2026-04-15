#include <stdio.h>
#include "serial.h"
#include "dartt_sync.h"
#include "args.h"
#include "dartt_bl.h"
#include <elfio/elfio.hpp>
#include "callbacks.h"
#include "version.h"
#include "dartt_flasher.h"

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
		int rc = flasher.read_to_file(args.output_file, rorigin, rlen);
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

	//todo: extract file extension and split based on .bin/.elf. If invalid throw error and exit. Exit if empty
	if(args.filename == NULL)
	{
		printf("Error: no file specified\n");
		return 1;
	}

	if(args.verify_only)
	{
		if(args.has_origin_addr == false)
		{
			int rc = flasher.get_bin_crc(args.filename, crc32);
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
			int rc = flasher.readback_verification(args.filename, args.origin_addr);
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
	int rc = 0;
	if(args.has_origin_addr == false)
	{
		rc = flasher.write_bin(args.filename, !args.no_verify);
	}
	else
	{
		rc = flasher.write_bin(args.filename, !args.no_verify, args.origin_addr);
	}
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

