#include "checksum.h"
#include "dartt_bl.h"
#include "unity.h"


void test_bl_init(void)
{
	dartt_bl_init(NULL);
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
}

