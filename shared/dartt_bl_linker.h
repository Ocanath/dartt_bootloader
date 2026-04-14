#ifndef DARTT_BL_LINKER_H
#define DARTT_BL_LINKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @defgroup dartt_bl_linker Linker Symbol Accessors
 * @brief Weak accessor functions for linker-defined flash region symbols.
 *
 * On hardware, the weak definitions in dartt_bl_linker.c resolve the symbols using
 * the @c extern unsigned char[] array trick, which correctly yields the symbol address
 * without dereferencing it. In the test environment, test/support/test_stubs.c provides
 * strong definitions that return addresses into @c fake_application_area, displacing the
 * weak defaults at link time.
 * @{
 */

/**
 * @brief Get the base address of target flash memory.
 * @return Pointer to the first byte of flash.
 */
const unsigned char * dartt_bl_get_flash_base(void);

/**
 * @brief Get the start address of the application region in flash.
 * @return Pointer to the first byte of the application flash region.
 */
const unsigned char * dartt_bl_get_app_start(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
