#ifndef DARTT_BL_STUBS_H
#define DARTT_BL_STUBS_H

#include "dartt_bl.h"

/** @defgroup dartt_bl_stubs Target Implementation Stubs
 *  @brief All symbols in this group must be defined by the target implementation. See target_implementation_guide.md.
 *  @{
 */

/** @brief First address of flash on the target. Defined by linker script. */
extern const unsigned char * application_start_addr__;

/** @brief Base address of flash memory on the target. Defined by linker script. */
extern const unsigned char * flash_base_addr__;

/**
 * @brief Load persistent settings from non-volatile storage into @c pbl->fds.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_load_fds(dartt_bl_t * pbl);

/**
 * @brief Populate @c pbl->attr with target flash attributes.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 * @note Must set nonzero values for both @c attr.page_size and @c attr.write_size.
 */
uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl);

/**
 * @brief Write @c pbl->fds to non-volatile storage.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_update_persistent_settings(dartt_bl_t * pbl);

/**
 * @brief Poll all active comm interfaces and update @c pbl from any received DARTT messages.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 * @note Must call @c dartt_frame_to_payload() and, on success, @c dartt_parse_general_message().
 */
uint32_t dartt_bl_handle_comms(dartt_bl_t * pbl);

/**
 * @brief De-initialize peripherals before jumping to application. Must disable interrupts.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_cleanup_system(void);

/**
 * @brief Set VTOR, load MSP, and jump to application code at @c application_start_addr__.
 * @param pbl Pointer to bootloader control structure.
 * @return Does not return on success.
 */
uint32_t dartt_bl_start_application(dartt_bl_t * pbl);

/**
 * @brief Write @c pbl->working_buffer to the address returned by @c dartt_bl_get_working_ptr().
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 * @note @c working_size is guaranteed to be a nonzero multiple of @c attr.write_size.
 */
uint32_t dartt_bl_flash_write(dartt_bl_t * pbl);

/**
 * @brief Erase @c pbl->erase_num_pages pages starting at the address from @c dartt_bl_get_page_addr().
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_flash_erase(dartt_bl_t * pbl);

/** @} */

#endif
