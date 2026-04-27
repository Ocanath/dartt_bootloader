# Target Implementation Guide

This document covers everything required to bring up a new bootloader target. All target-specific work lives in `bootloaders/<target>/`. The `shared/` directory must not be modified.

---

## 1. Linker Symbols

The following symbols must be defined by the linker script:

```
/*Our definitions.*/
PAGE_SIZE = 0x800;		//implementation specific, set to correct value for proper functionality
BOOTLOADER_SIZE = 22K;	//or 12K, or whatever it needs to be to fit the bootloader application

/*
Flash memory region definition. Define ram, magic word, and flash regions
*/
MEMORY
{
	RAM	(xrw)	: ORIGIN = 0xXXXXXXXX, LENGTH = (X - 4)
	MAGIC_WORD	(xrw)	: ORIGIN = ORIGIN(RAM) + LENGTH(RAM), LENGTH = 4
	FLASH	(rx)	:	ORIGIN=0xXXXXXXXX, LENGTH=BOOTLOADER_SIZE
}

flash_base_addr__ = ORIGIN(FLASH);
application_start_addr__ = ORIGIN(FLASH) + BOOTLOADER_SIZE + PAGE_SIZE;
ram_blockstart_keyword_addr__ = ORIGIN(MAGIC_WORD);
```
This defines the memory layout. The `BOOTLOADER_SIZE` may vary depending on the application. You *must* ensure that the application start address of the target to be flashed starts at the application start addr, or it will not work.

`flash_base_addr__` and `application_start_addr__` are externed in `dartt_bl_linker.c` and must be compile-time constants resolved by the linker. They are not variables — do not define them in C. Linker is source of truth for memory layout information.

**`flash_base_addr__`** — the first address of flash memory on the target. On STM32 this is typically `0x08000000`.

**`application_start_addr__`** — the first valid address for application code. This is `flash_base_addr__ + bootloader_size`. The bootloader partition size must be fixed and consistent across all applications targeting this bootloader build. All application linker scripts must use this same value as their flash origin.


These two symbols are used by the shared core for bootloader region protection on erase and write. Getting them wrong will either fail to protect the bootloader or block valid application operations.

The final symbol `ram_blockstart_keyword_addr__` is the 'magic word' address that persists across a system reset. It is retrieved via a weak stub and can be overridden. It is checked exactly once, if and only if the persistent settings `boot_mode` is equal to the startup key. If `boot_mode` is equal to the key, and the magic word is **also equal** to the startup key, the bootloader does not automatically jump to the application - it will continue running the event loop until a START command was sent. This exists to allow auto-boot to application, with a recovery mechanism to prevent soft lockout of the bootloader.

**IMPORTANT:** The application linker script must mirror this: the start address in its linker script must match `application_start_addr__`, the flash length must be reduced accordingly, and the reserved RAM address `ram_blockstart_keyword_addr__` must be reserved at the same location as well for that mechanism to work. If `ram_blockstart_keyword_addr__` will always be unused it is possible to overload the weak function definition and create an application-defined `dartt_bl_get_ram_blockstart_word()` implementation that always returns 0 (soft-bricking the bootloader unless you can beat the race) or the key value (overriding the auto-start behavior completely).

Example application linker script definition: 

``` 
/*Our definitions.*/
PAGE_SIZE = 0x800;
BOOTLOADER_SIZE = 22K;

/* Memories definition */
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = (32K-4)
  MAGIC_WORD (xrw) : ORIGIN = ORIGIN(RAM)+LENGTH(RAM),	LENGTH = 4
  BOOTLOADER (rx) : ORIGIN = 0x8000000,	LENGTH = BOOTLOADER_SIZE
  FLASH    (rx)    : ORIGIN = ORIGIN(BOOTLOADER) + BOOTLOADER_SIZE + PAGE_SIZE,   LENGTH = (128K - (BOOTLOADER_SIZE + PAGE_SIZE) )	
}
```

Remember that the layout is always:

**Flash:**
1. Bootloader (reserve however much memory is necessary, typically 10-20KB)
1. Bootloader settings (always exactly 1 page)
1. Application defined. Can include additional reserved pages if necessary

**RAM**
1. Normal memory (i.e. default) up to the last available word
1. Exactly one word for blocking auto-start.

### 1.1 Linker Memory Map

On an STM32, the linker script must also be modified to have proper definitions of the memory map. The bootloader size is defined there (i.e. 16K, etc). As stated above, the flash base and application start symbols must be defined, but also you **must** check whether or not the total bootloader region (bootloader program + single reserved persistent settings page) overruns into the application area. 

On your target application, this is also where you define the target start memory location. application_start_addr__ in the target bootloader must match the start address in your .elf or the bootloader tool should error out.

---

## 2. Stub Functions

All functions declared in `dartt_bl_stubs.h` must be implemented. The bootloader will not link without them. Each is described below.

### `dartt_bl_get_attributes(dartt_bl_t * pbl)`

Populate `pbl->attr`. All three fields must be set to nonzero values — the bootloader checks this during init and returns `DARTT_BL_INITIALIZATION_FAILURE` if any is zero.

```c
pbl->attr.page_size  // erasable page size in bytes
pbl->attr.write_size // minimum write granularity in bytes (must be power of 2)
pbl->attr.num_pages  // total number of pages on the target
```

Example (STM32G431, 2KB pages, 8-byte doubleword writes, 64 pages):
```c
pbl->attr.page_size  = 2048;
pbl->attr.write_size = 8;
pbl->attr.num_pages  = 64;
```

### `dartt_bl_handle_comms(dartt_bl_t * pbl)`

Handle all active communication interfaces and update `pbl` from any received DARTT messages. Called on every iteration of the event handler loop. Must:

1. Poll or drain the receive buffer for each active interface (UART, CAN, etc.)
2. Unframe/unstuff incoming bytes (e.g. COBS)
3. Call `dartt_frame_to_payload()` to extract the payload
4. On success, call `dartt_parse_general_message()` to update `pbl`

On little-endian 32-bit targets, `pbl` *can* be passed directly as the DARTT register map with no additional marshalling. This is not standard compliant but is generally okay when the target implementation is known to correctly lay out the map.

### `dartt_bl_flash_erase(uint32_t erase_page, uint32_t erase_num_pages)`

Erase `erase_num_pages` pages starting at the absolute page index `erase_page`. The byte address of the first page to erase is `flash_base + erase_page * page_size`.

The shared core has already validated the request (nonzero page size, nonzero num pages, address above `application_start_addr__`) before this stub is called.

### `dartt_bl_flash_write(unsigned char * dest, unsigned char * src, size_t size)`

Write `size` bytes from `src` to `dest` in target flash. `dest` is the target byte address, `src` is the source data buffer, and `size` is guaranteed to be a nonzero multiple of `attr.write_size`. The shared core has already validated the request before this stub is called.

### `dartt_bl_cleanup_system(void)`

De-initialize peripherals before jumping to the application. At minimum, disable interrupts (`__disable_irq()`). Deinitialize any peripherals that the application does not expect to inherit initialized state for. Called immediately before `dartt_bl_start_application`.

### `dartt_bl_start_application(dartt_bl_t * pbl)`

Jump to application code. The standard sequence:

1. `__disable_irq()`
2. Set `SCB->VTOR` to `(uint32_t)application_start_addr__`
3. Load MSP from the first word of the application vector table: `__set_MSP(*(uint32_t *)application_start_addr__)`
4. Load the reset handler address from the second word and jump: `((void (*)(void))(*(uint32_t *)(application_start_addr__ + 4)))()`


The startup assembly defines the vector table contents but does not depend on an absolute address in flash memory. Setting the VTOR pointer is sufficient for properly mapping interrupts in a target with modified ORIGIN. The target application linker script must be modified so the ORIGIN matches the application start address. This should be checked by the flashing tool and throw an error if there's an overlap.

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
