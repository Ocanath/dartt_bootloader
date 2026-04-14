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
	if(args.get_version)
	{
		printf("Flashing Tool Version: %s\n", firmware_version);
	}

	DarttFlasher flasher(args.dartt_address);
	flasher.ser.autoconnect(921600);


	if(args.get_version)
	{
		std::string version;
		int rc = flasher.get_version(version);	
		if(rc == 0)
		{
			printf("Bootloader Device Version: ");
			printf("%s\n",version.c_str());
		}
		else
		{
			printf("Error: code %d\n", rc);
			return rc;
		}
	}
	int rc = flasher.write_bin(NULL, 0);
	if(rc != 0)
	{
		printf("Error %d\n", rc);
	}
	
	return rc;
}

