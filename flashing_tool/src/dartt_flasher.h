#ifndef DARTT_FLASHER_H
#define DARTT_FLASHER_H

#include "dartt_sync.h"

class DarttFlasher
{
	public:
		DarttFlasher(unsigned char addr);
		~DarttFlasher();
	private:
		dartt_sync_t ds;
};

#endif
