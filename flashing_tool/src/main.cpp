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
	flasher.bootloader_control.action_flag = START_APPLICATION;
	dartt_buffer_t ctl = 
	{
		.buf = (unsigned char *)(&flasher.bootloader_control.action_flag), 
		.size = sizeof(uint32_t), 
		.len = sizeof(uint32_t)
	};
	dartt_sync(&ctl, &flasher.ds);
}
