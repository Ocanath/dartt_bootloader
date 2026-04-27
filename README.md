# DARTT Bootloader

This project aims to create a generic bootloader based on DARTT. The DARTT interface should theoretically be wire-able with relatively little effort to other microcontrollers and interfaces. 

## Building the Flashing Tool

```bash
mkdir build && cd build
cmake ..
make dartt_flash
```

```bash
cp dartt_flash ~/.local/bin/  # optional: install to user PATH
```

## Basic Hardware/Peripheral Requirements

1. Access to nonvolatile storage (read and write) for reading and storing the primary device address.

1. Access to the interface over which DARTT will be routed, such as UART, SPI, FDCAN, etc. A Serial based flashing tool is provided in this repository.

## Code Architecture

The bootloader core is implemented in `shared/`. The core bootloader logic and call sites are defined there. All hardware facing functions will be declared in shared with no definition - it is up to the specific project that links them to define the stubs.

This project will have a set number of dedicated bootloader projects that target a specific hardware platform and link to `shared/`. Each bootloader sub-project will be a fully functional & tested bootloader program that compiles to an executable binary for the target platform.

Executable projects will be in `bootloaders/`. 

## Functions

The bootloader will have a deferred action interface which allows the triggering of certain commands. These can include:

- Start the application
- Read memory from a specific location into the buffer
- Write the buffer to a specific memory location
- Trigger an erasure of the currently selected page(s). 
- Compute the CRC32 of the application image, for verification
- Save persistent settings
- Get the git version hash of the bootloader
- Read pointers (application start, working buffer) as well as read/write to the target pointer for flash read and write operations

Actions dispatched via the action register upon completion will load their return code into `action_status`, indicating success or failure, and indicate completion by setting `action_flag` to `NO_ACTION`. `action_status` is considered stale if `action_flag` is nonzero, and always corresponds to the most recently completed dispatched action.

Certain memory operations may trigger a HardFault. **The recommended action on hardfault is always to reset.** After reset, `dartt_bl_init()` sets `action_status` to `DARTT_BL_INITIALIZED`, which the flashing tool uses to detect that a fault occurred during the previous operation.

### Pointers

The application start point should be checked by the flashing tool to make sure it matches the expected value from the application binary. The application start pointer is a compile-time constant resolved by the linker script via `application_start_addr__`. The bootloader partition size is fixed per target and defined in its linker script.

The flashing tool should throw a code and exit if the bootloader start address does not match the start address of the target to flash.

Retrieval of the application start address shall be done with the following steps:

- `dartt_write_multi` call to assign `.action_flag = GET_APPLICATION_START_ADDR;`
- Poll `.action_status` via `dartt_read_multi`. If it returns `DARTT_BL_SUCCESS` the contents of the working buffer are valid
- Read `.working_size` AND `.working_buffer` via `dartt_read_multi`. This retrieves the target architecture size as well. Write the `.working_buffer` contents into a `uintptr_t` and validate it against the contents of the binary file to flash.

### Working buffer

The core flashing operations are routed through the `.working_buffer`. The `.working_buffer` is comprised of three different values:

- `.working_size`
- `.working_buffer`
- `working_target_ptr_`

#### Reading

`working_target_ptr_` defines the working location for the working buffer. When the working buffer is used for reads, a `READ_BUFFER` operation loads `.working_size` bytes into the `.working_buffer`, from `working_target_ptr_` to `working_target_ptr_ + .working_size`. 

There is no explicitly forbidden region of memory for reads. The target may hardfault - it is the implementer's responsibility to ensure hardfaults trigger a reset operation for proper bootloader recovery.

#### Writing

Similarly, the writing range on the target is from `working_target_ptr_` to `working_target_ptr_ + .working_size`. The `WRITE_BUFFER` abstracts flash write calls on the target architecture through the range and sets `.action_status` and `.action_flag` on completion, as with all deferred actions. Flash writes are target specific, and the stub must be implemented for each specific target.

There is one address range explicitly blocked by the bootloader itself: the bootloader region (from target start address, `0x08000000` on STM32, up to `application_start_addr__`). Updates to the persistent settings have a convenience deferred action `SAVE_SETTINGS`, but those settings can be explicitly overwritten by the bootloader if persistent storage restructure is required in future versions.

Attempted writes to illegal memory-mapped regions can result in a hardfault. There is no explicit protection from hardfaults in this protocol - it is the implementer's responsibility to ensure hardfaults result in a reset for proper bootloader recovery.

### Erasure

Erasure works differently. The page attributes must be read from `.page_size`. The flashing tool must set `.erase_page` and `.erase_num_pages` before running the deferred erase operation via `ERASE_PAGES`. This calls a stub which must be implemented on each specific target.

Erasure is explicitly forbidden for the bootloader memory region, same as with writes. This is enforced by the event loop - request validity does not have to be enforced by the stub implementation, (although redundancy is not necessarily a bad thing).

**Note:** Page indexing is absolute on the target memory.

### Verification

The deferred `GET_CRC32` action calculates a crc32 value from `application_start_addr__` to `application_start_addr__ + .application_size`. The flashing tool must set `.application_size` and save for proper startup flash validation. 

## Boundary checks

The bootloader flash region boundary is defined entirely by the linker script. Three symbols are resolved at link time and accessed via weak accessor functions in `dartt_bl_linker.c`:

```c
extern const unsigned char flash_base_addr__[];
extern const unsigned char application_start_addr__[];
extern const unsigned char ram_blockstart_keyword_addr__[];
```

`application_start_addr__` is always the first valid address for application code and defines the bootloader protection boundary for erase and write operations. `ram_blockstart_keyword_addr__` is the address of the RAM sentinel word used for runtime bootloader re-entry (see `doc/runtime_bootloader_entry.md`).

The application start is always set as the bootloader boundary/end.

**IMPORTANT NOTE:** We must set a fixed bootloader partition size. All applications using the bootloader will have to have their linker scripts updated with the correct start address: `0x08000000 + bootloader_size`. This has to happen because address offsetting is nontrivial - things like jump tables, function pointers, NVIC, are addressed relative to base, so it's not as simple as just subtract an offset from every write location - the actual flash data in certain scenarios is offsetted from the base. 

More notes on how to handle this with debugging workflow, etc. to follow. SWD debuggability should be possible if the bootloader is configured with the magic word to auto-launch, theoretically. 