#include "dartt_bl.h"
#include "dartt_bl_stubs.h"


/*Initialize the bootloader. calls unimplemented helper functions*/
uint32_t dartt_bl_init(dartt_bl_t * pbl)
{
	//initialize the filesystem
	uint32_t rc = dartt_bl_load_fds(pbl);
	if(rc != DARTT_BL_SUCCESS)
	{
		return rc;
	}
	//initialize attributes
	rc = dartt_bl_get_attributes(pbl);
	if(rc != DARTT_BL_SUCCESS)
	{
		return rc;
	}
}

/*Main event handler. called in a loop.*/
void dartt_bl_event_handler(dartt_bl_t * pbl)
{
	dartt_bl_handle_comms(pbl);
	if(pbl->action_flag != NO_ACTION)
	{
		switch (pbl->action_flag)
		{
			case(START_APPLICATION):
			{
				break;
			}
			case(SAVE_SETTINGS):
			{
				pbl->action_status = dartt_bl_update_filesystem(pbl);
				break;
			}
			default:
			{
				break;
			}
		}
	}	
}
