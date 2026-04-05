# Target Implementation Guide

This document covers everything required to bring up a new bootloader target. All target-specific work lives in `bootloaders/<target>/`. The `shared/` directory must not be modified.

---

## 1. Linker Symbols

Two symbols must be defined by the linker script:

```
application_start_addr__
flash_base_addr__
```

These are declared as `extern const unsigned char *` in `dartt_bl_stubs.h` and must be compile-time constants resolved by the linker. They are not variables — do not define them in C.

**`flash_base_addr__`** — the first address of flash memory on the target. On STM32 this is typically `0x08000000`.

**`application_start_addr__`** — the first valid address for application code. This is `flash_base_addr__ + bootloader_size`. The bootloader partition size must be fixed and consistent across all applications targeting this bootloader build. All application linker scripts must use this same value as their flash origin.

Example (GCC linker script):
```
flash_base_addr__        = 0x08000000;
application_start_addr__ = 0x08004000;  /* 16KB bootloader partition */
```

These two symbols are used by the shared core for bootloader region protection on erase and write. Getting them wrong will either fail to protect the bootloader or block valid application operations.

---

## 2. Stub Functions

All functions declared in `dartt_bl_stubs.h` must be implemented. The bootloader will not link without them. Each is described below.

### `dartt_bl_get_attributes(dartt_bl_t * pbl)`

Populate `pbl->attr`. Both fields must be set to nonzero values — the bootloader checks this during init and returns `DARTT_BL_INITIALIZATION_FAILURE` if either is zero.

```c
pbl->attr.page_size  // erasable page size in bytes
pbl->attr.write_size // minimum write granularity in bytes (must be power of 2)
```

Example (STM32G431, 2KB pages, 8-byte doubleword writes):
```c
pbl->attr.page_size  = 2048;
pbl->attr.write_size = 8;
```

### `dartt_bl_load_fds(dartt_bl_t * pbl)`

Load the persistent settings structure from non-volatile storage into `pbl->fds`. The `dartt_bl_persistent_t` layout must match what was written by `dartt_bl_update_persistent_settings`. On first boot (blank flash), handle the unprogrammed state gracefully and return `DARTT_BL_SUCCESS` with zeroed/default values.

### `dartt_bl_update_persistent_settings(dartt_bl_t * pbl)`

Write `pbl->fds` to non-volatile storage. The storage location and mechanism are target-specific. Ensure the region used does not overlap the application partition or the bootloader itself.

### `dartt_bl_handle_comms(dartt_bl_t * pbl)`

Handle all active communication interfaces and update `pbl` from any received DARTT messages. Called on every iteration of the event handler loop. Must:

1. Poll or drain the receive buffer for each active interface (UART, CAN, etc.)
2. Unframe/unstuff incoming bytes (e.g. COBS)
3. Call `dartt_frame_to_payload()` to extract the payload
4. On success, call `dartt_parse_general_message()` to update `pbl`

On little-endian 32-bit targets, `pbl` *can* be passed directly as the DARTT register map with no additional marshalling. This is not standard compliant but is generally okay when the target implementation is known to correctly lay out the map.

### `dartt_bl_flash_erase(dartt_bl_t * pbl)`

Erase `pbl->erase_num_pages` pages starting at the page index given by `pbl->erase_page`. Use `dartt_bl_get_page_addr(pbl)` to compute the start address — do not recompute it manually.

The shared core has already validated the request (nonzero page size, nonzero num pages, address above `application_start_addr__`) before this stub is called.

### `dartt_bl_flash_write(dartt_bl_t * pbl)`

Write `pbl->working_size` bytes from `pbl->working_buffer` to the address returned by `dartt_bl_get_working_ptr()`. The shared core has already validated the request before this stub is called.

`pbl->working_size` is guaranteed to be a multiple of `pbl->attr.write_size`.

### `dartt_bl_cleanup_system(void)`

De-initialize peripherals before jumping to the application. At minimum, disable interrupts (`__disable_irq()`). Deinitialize any peripherals that the application does not expect to inherit initialized state for. Called immediately before `dartt_bl_start_application`.

### `dartt_bl_start_application(dartt_bl_t * pbl)`

Jump to application code. The standard sequence:

1. `__disable_irq()`
2. Set `SCB->VTOR` to `(uint32_t)application_start_addr__`
3. Load MSP from the first word of the application vector table: `__set_MSP(*(uint32_t *)application_start_addr__)`
4. Load the reset handler address from the second word and jump: `((void (*)(void))(*(uint32_t *)(application_start_addr__ + 4)))()`

---

## 3. Hard Fault Handler

The bootloader performs arbitrary memory reads and writes which can trigger hard faults on invalid addresses. The hard fault handler **must** trigger a system reset — an infinite loop is not acceptable as it prevents the flashing tool from detecting the fault.

```c
void HardFault_Handler(void)
{
    NVIC_SystemReset();
}
```

After reset, `dartt_bl_init()` sets `action_status = DARTT_BL_INITIALIZED`. The flashing tool detects this and knows the previous operation caused a fault.

---

## 4. Main Loop

```c
int main(void)
{
    // hardware init (clocks, peripherals, comms)

    dartt_bl_t bootloader = {};
    dartt_bl_init(&bootloader);

    while (1)
    {
        dartt_bl_event_handler(&bootloader);
    }
}
```

`dartt_bl_init` must be called before the event handler loop. Do not call the event handler before init completes — `action_status` is undefined prior to init.


## 5. version.h

The target implementation needs to generate the version header with pre-build scripting. `get_version.bat` and `get_version.sh` are provided in `scripts/`. It is critical that these scripts run on build so that the hash is not stale.

It is recommended to use the ruby script to do this, but it is not essential - ruby is used because it is required to run the unit test suite (ceedling).
