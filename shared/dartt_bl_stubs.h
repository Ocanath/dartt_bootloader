#ifndef DARTT_BL_STUBS_H
#define DARTT_BL_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"
#include "dartt_bl.h"

/** @defgroup dartt_bl_stubs Target Implementation Stubs
 *  @brief All symbols in this group must be defined by the target implementation. See target_implementation_guide.md.
 *  @{
 */

/**
 * @brief Populate @c pbl->attr with target flash attributes.
 * @param pbl Pointer to bootloader control structure.
 * @return @c DARTT_BL_SUCCESS or error code.
 * @note Must set nonzero values for both @c attr.page_size and @c attr.write_size.
 */
uint32_t dartt_bl_get_attributes(dartt_bl_t * pbl);

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
 * @brief Write @c size bytes from @c src to @c dest in target flash.
 * @param dest  Target flash address to write to.
 * @param src   Source buffer containing data to write.
 * @param size  Number of bytes to write. Guaranteed to be a nonzero multiple of @c attr.write_size.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size);

/**
 * @brief Erase @c erase_size bytes of flash starting at @c page_addr.
 * @param page_addr  Byte address of the first page to erase. Aligned to @c attr.page_size.
 * @param erase_size Total bytes to erase. Equal to @c erase_num_pages * @c attr.page_size.
 * @return @c DARTT_BL_SUCCESS or error code.
 */
uint32_t dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
