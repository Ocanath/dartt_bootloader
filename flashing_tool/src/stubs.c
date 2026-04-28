#include "dartt_bl.h"
#include "dartt_bl_stubs.h"
#include "dartt_bl_linker.h"

/*Empty/dummy implementation of stubs for access to helpers in the bootloader core*/
const unsigned char * dartt_bl_get_flash_base(void) { return (const unsigned char *)0x00; }
const unsigned char * dartt_bl_get_app_start(void)  { return (const unsigned char *)0x00; }
const uint32_t dartt_bl_get_ram_blockstart_word(void) {return 0;}
uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl){return 0;}
uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl){return 0;}
uint32_t dartt_bl_cleanup_system(void){return 0;}
uint32_t dartt_bl_start_application(dartt_bl_t * pbl){return 0;}
uint32_t dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size){return 0;}
uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages){return 0;}
