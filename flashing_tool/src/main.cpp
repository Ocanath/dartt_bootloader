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
	flasher.ser.autoconnect(921600);

	std::string version;
	int rc = flasher.get_version(version);	
	if(rc == 0)
	{
		printf("Got version success\n");
		printf("%s\n",version.c_str());
	}
	else
	{
		printf("Error: code %d\n", rc);
	}
	return 0;
}

