#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


args_t parse_args(int argc, char** argv)
{
	return (args_t){.filename = "", .dartt_address = 255, .baudrate = 921600};
}
