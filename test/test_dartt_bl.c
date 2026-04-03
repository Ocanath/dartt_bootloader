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

void test_getcrc32(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(0, bootloader_ctl.fds.application_crc32);
	bootloader_ctl.action_flag = GET_CRC32;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_status);
	TEST_ASSERT_NOT_EQUAL(0, bootloader_ctl.fds.application_crc32);	//assume correct crc32 calculation from dartt tests
}