#include "checksum.h"
#include "dartt_bl.h"
#include "dartt_bl_stubs.h"
#include "unity.h"

extern unsigned char fake_application_area[0x2000];	

void test_bl_init(void)
{
	dartt_bl_init(NULL);
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
}

void test_get_app_start(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);

	TEST_ASSERT_EQUAL(0, bootloader_ctl.working_size);
	bootloader_ctl.action_flag = GET_APPLICATION_START_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	TEST_ASSERT_EQUAL(sizeof(unsigned char *), bootloader_ctl.working_size);
	uintptr_t p = 0;
	for(int i = 0; i < sizeof(unsigned char *); i++)
	{
		int shift = i*8;
		p |= ((uintptr_t)(bootloader_ctl.working_buffer[i])) << shift;
	}
	uintptr_t startref = (uintptr_t)(application_start_addr__);
	TEST_ASSERT_EQUAL(startref, p);
}

void test_wbuf_helpers(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);

	unsigned char * new_target = &fake_application_area[2];
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	//this function assigns working_size to the correct value. dartt_sync or dartt_write_multi will properly update target - we abstract that here with direct access to the struct
	uint32_t rc = dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, new_target);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, rc);
	unsigned char * check_target = NULL;
	rc = dartt_bl_load_wbuf_to_ptr(&bootloader_ctl, &check_target);
	TEST_ASSERT_EQUAL(new_target, check_target);

	bootloader_ctl.working_size = 2;
	rc = dartt_bl_load_wbuf_to_ptr(&bootloader_ctl, &check_target);
	TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)rc);


}

void test_set_get_working_pointer(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);


	TEST_ASSERT_EQUAL(0, bootloader_ctl.working_size);
	unsigned char * new_target = &fake_application_area[5];
	//working size already has the correct value and we checked it above with an assert
	uint32_t rc = dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, new_target);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, rc);

	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	for(int i = 0; i < sizeof(unsigned char *); i++)
	{
		bootloader_ctl.working_buffer[i] = 0;
	}

	bootloader_ctl.action_flag = GET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	unsigned char * check_target = NULL;
	rc = dartt_bl_load_wbuf_to_ptr(&bootloader_ctl, &check_target);
	TEST_ASSERT_EQUAL(new_target, check_target);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, rc);


}


void test_read_buffer(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);

	for(size_t i = 0; i < sizeof(fake_application_area); i++)
	{
		fake_application_area[i] = i+1;
	}

	{//test case 1 - working size = 0, should return error 
		bootloader_ctl.working_size = 0;
		bootloader_ctl.action_flag = READ_BUFFER;
		dartt_bl_event_handler(&bootloader_ctl);
		TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
		TEST_ASSERT_EQUAL(DARTT_BL_BAD_READ_REQUEST, (int32_t)bootloader_ctl.action_status);
	}
	{//test case 2 - working size too big, should return error 
		bootloader_ctl.working_size = sizeof(bootloader_ctl.working_buffer) + 1;
		bootloader_ctl.action_flag = READ_BUFFER;
		dartt_bl_event_handler(&bootloader_ctl);
		TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
		TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)bootloader_ctl.action_status);
	}

	{//test case 3 - valid read. this should be handled carefully, because you could crash within ceedling. has to have a valid pointer
		
		//first, load the target address pointer into the working buffer
		dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, &fake_application_area[77]);
		//implied dartt_read_multi + dartt_sync call here
		//next, run the deferred action handler to load the pointer
		bootloader_ctl.action_flag = SET_WORKING_ADDR;
		//ipmlied dartt_sync call here
		dartt_bl_event_handler(&bootloader_ctl);
		//implied status polling here
		TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
		TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
		//assume correctness of working pointer assignment from prior tests.

		//next, set request size 
		bootloader_ctl.working_size = 44;
		bootloader_ctl.action_flag = READ_BUFFER;
		dartt_bl_event_handler(&bootloader_ctl);
		TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
		TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, (int32_t)bootloader_ctl.action_status);
		for(size_t i = 0; i < 44; i++)
		{
			TEST_ASSERT_EQUAL(fake_application_area[i+77], bootloader_ctl.working_buffer[i]);
		}
	}
}


void test_getcrc32(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(0, bootloader_ctl.fds.application_crc32);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	
	bootloader_ctl.action_flag = GET_CRC32;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	TEST_ASSERT_NOT_EQUAL(0, bootloader_ctl.fds.application_crc32);	//assume correct crc32 calculation from dartt tests
}


