#include <stdbool.h>
#include "checksum.h"
#include "dartt_bl.h"
#include "dartt_bl_stubs.h"
#include "version.h"
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
	set_application_size(&bootloader_ctl);

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
	set_application_size(&bootloader_ctl);

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
	set_application_size(&bootloader_ctl);

	TEST_ASSERT_EQUAL(0, bootloader_ctl.working_size);
	unsigned char * new_target = &fake_application_area[5];
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

void test_read_buffer_size_zero(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.working_size = 0;
	bootloader_ctl.action_flag = READ_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_BAD_READ_REQUEST, (int32_t)bootloader_ctl.action_status);
}

void test_read_buffer_size_too_large(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.working_size = sizeof(bootloader_ctl.working_buffer) + 1;
	bootloader_ctl.action_flag = READ_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)bootloader_ctl.action_status);
}

void test_read_buffer_valid(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	for(size_t i = 0; i < sizeof(fake_application_area); i++)
	{
		fake_application_area[i] = i+1;
	}

	//implied dartt_read_multi + dartt_sync call here
	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, &fake_application_area[77]);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 44;
	bootloader_ctl.action_flag = READ_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, (int32_t)bootloader_ctl.action_status);
	for(size_t i = 0; i < 44; i++)
	{
		TEST_ASSERT_EQUAL(fake_application_area[i+77], bootloader_ctl.working_buffer[i]);
	}
}

void test_erase_invalid_numpages(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	TEST_ASSERT_NOT_EQUAL(0, bootloader_ctl.attr.page_size);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.action_flag = ERASE_PAGES;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_ERASE_FAILED_INVALID_NUMPAGES, (int32_t)bootloader_ctl.action_status);
}

void test_erase_blocked(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.erase_num_pages = 1;
	bootloader_ctl.erase_page = 0;
	bootloader_ctl.action_flag = ERASE_PAGES;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_ERASE_BLOCKED, (int32_t)bootloader_ctl.action_status);

	bootloader_ctl.erase_num_pages = 1;
	bootloader_ctl.erase_page = 2;
	bootloader_ctl.action_flag = ERASE_PAGES;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_ERASE_BLOCKED, (int32_t)bootloader_ctl.action_status);

}

void test_erase_valid(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	for(size_t i = 0; i < sizeof(fake_application_area); i++)
	{
		fake_application_area[i] = i+1;
	}

	bootloader_ctl.erase_num_pages = 1;
	bootloader_ctl.erase_page = 4;
	bootloader_ctl.action_flag = ERASE_PAGES;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	unsigned char * erase_ptr = (unsigned char *)dartt_bl_get_page_addr(&bootloader_ctl);
	TEST_ASSERT_EQUAL(application_start_addr__, erase_ptr);
	for(size_t i = 0; i < bootloader_ctl.attr.page_size; i++)
	{
		TEST_ASSERT_EQUAL(0xFF, application_start_addr__[i]);
	}
}

void test_write_buffer_null_pointer(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.working_size = 8;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_NULLPTR, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_size_zero(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, application_start_addr__);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 0;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_size_too_large(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, application_start_addr__);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = sizeof(bootloader_ctl.working_buffer) + 1;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_size_not_multiple_of_write_size(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, application_start_addr__);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 7;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WORKING_SIZE_INVALID, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_write_size_uninit(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, application_start_addr__);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.attr.write_size = 0;
	bootloader_ctl.working_size = 8;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WRITE_SIZE_UNINITALIZED, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_blocked(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, flash_base_addr__);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 8;
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_WRITE_BLOCKED, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_valid(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	for(size_t i = 0; i < sizeof(fake_application_area); i++)
	{
		fake_application_area[i] = 0xFF;
	}

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, &fake_application_area[0x900]);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 8;
	for(int i = 0; i < 8; i++)
	{
		bootloader_ctl.working_buffer[i] = 0xAA + i;
	}

	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	for(int i = 0; i < 8; i++)
	{
		TEST_ASSERT_EQUAL(0xAA + i, fake_application_area[0x900 + i]);
	}
}

void test_read_buffer_out_of_bounds(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, &fake_application_area[0x1FF0]);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 32;	// 0x1FF0 + 32 = 0x2010 > 0x2000
	bootloader_ctl.action_flag = READ_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_OUT_OF_BOUNDS, (int32_t)bootloader_ctl.action_status);
}

void test_write_buffer_out_of_bounds(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	dartt_bl_load_ptr_to_wbuf(&bootloader_ctl, &fake_application_area[0x1FF8]);
	bootloader_ctl.action_flag = SET_WORKING_ADDR;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	bootloader_ctl.working_size = 16;	// 0x1FF8 + 16 = 0x2008 > 0x2000
	bootloader_ctl.action_flag = WRITE_BUFFER;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_OUT_OF_BOUNDS, (int32_t)bootloader_ctl.action_status);
}

void test_erase_out_of_bounds(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.erase_page = 30;
	bootloader_ctl.erase_num_pages = 4;	// 30 + 4 = 34 > 32
	bootloader_ctl.action_flag = ERASE_PAGES;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_OUT_OF_BOUNDS, (int32_t)bootloader_ctl.action_status);
}

void test_getcrc32_out_of_bounds(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.fds.application_size = 0x1C01;	// 0x400 + 0x1C01 = 0x2001 > 0x2000
	bootloader_ctl.action_flag = GET_CRC32;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION, bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_OUT_OF_BOUNDS, (int32_t)bootloader_ctl.action_status);
}

void test_getcrc32(void)
{
	dartt_bl_t bootloader_ctl = {0};
	for(size_t i = 0; i < 0x400; i++)
	{
		fake_application_area[i] = 0x1;		//force all bootloader region stuff to be 0. hardcoded - check setup in stubs 
	}
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(0x01010101, bootloader_ctl.fds.application_crc32);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	bootloader_ctl.action_flag = GET_CRC32;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);
	TEST_ASSERT_NOT_EQUAL(0, bootloader_ctl.fds.application_crc32);	//assume correct crc32 calculation from dartt tests
}

void test_get_version_hash(void)
{
	dartt_bl_t bootloader_ctl = {0};
	dartt_bl_init(&bootloader_ctl);
	TEST_ASSERT_EQUAL(DARTT_BL_INITIALIZED, bootloader_ctl.action_status);
	set_application_size(&bootloader_ctl);

	TEST_ASSERT_NOT_EQUAL(0, sizeof(firmware_version));
	bootloader_ctl.action_flag = GET_VERSION_HASH;
	dartt_bl_event_handler(&bootloader_ctl);
	TEST_ASSERT_EQUAL(NO_ACTION,bootloader_ctl.action_flag);
	TEST_ASSERT_EQUAL(DARTT_BL_SUCCESS, bootloader_ctl.action_status);

	TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*)firmware_version, (uint8_t*)bootloader_ctl.working_buffer, sizeof(firmware_version));
	TEST_ASSERT_EQUAL(sizeof(firmware_version), bootloader_ctl.working_size);

}
