# Runtime Bootloader Entry

## Overview

The bootloader supports two startup behaviors, controlled by the `boot_mode` field in persistent settings (`dartt_bl_persistent_t.boot_mode`):

- **`boot_mode != DARTT_BL_START_PROGRAM_KEY`** — bootloader stays in the event loop and waits for an explicit `START_APPLICATION` command over DARTT.
- **`boot_mode == DARTT_BL_START_PROGRAM_KEY`** — autolaunch is enabled. On init, the bootloader checks a RAM sentinel word and jumps to the application immediately unless the sentinel blocks it.

## RAM Sentinel

A single word at the top of RAM is reserved as a sentinel via the `MAGIC_WORD` linker region and the `ram_blockstart_keyword_addr__` symbol. It is read once during `dartt_bl_init()` via `dartt_bl_get_ram_blockstart_word()`, and only when `boot_mode` is the start key.

**Autolaunch proceeds** if `boot_mode == DARTT_BL_START_PROGRAM_KEY` and the sentinel word is anything other than `DARTT_BL_START_PROGRAM_KEY`.

**Autolaunch is blocked** if both `boot_mode` and the sentinel word equal `DARTT_BL_START_PROGRAM_KEY`. The bootloader stays in the event loop.

```c
if (pbl->fds.boot_mode == DARTT_BL_START_PROGRAM_KEY)
{
    if (dartt_bl_get_ram_blockstart_word() != DARTT_BL_START_PROGRAM_KEY)
    {
        pbl->action_flag = START_APPLICATION;
    }
}
```

## Application Re-entry Sequence

To force bootloader entry after a software reset, the application writes the key to the sentinel address before resetting:

```c
extern const unsigned char ram_blockstart_keyword_addr__[];
*(volatile uint32_t *)ram_blockstart_keyword_addr__ = DARTT_BL_START_PROGRAM_KEY;
NVIC_SystemReset();
```

The sentinel survives a software reset because the bootloader startup code never writes to the `MAGIC_WORD` region. On the next boot, the bootloader reads the sentinel, sees the key, and stays in the event loop.

On a cold power cycle, RAM content is indeterminate. The probability of the sentinel accidentally matching the 32-bit key is 1 in 2³², so autolaunch proceeds normally after a true power cycle.

## Application Linker Script Requirement

The application's linker script must mirror the `MAGIC_WORD` reservation so that the application's own BSS and stack do not alias that address. See `target_implementation_guide.md` for the required layout.

## Targets Without RAM Persistence

On targets where RAM does not survive a system reset (e.g. ECC SRAM requiring a startup scrub), override the weak `dartt_bl_get_ram_blockstart_word()` stub with a strong definition that always returns `DARTT_BL_START_PROGRAM_KEY`. This permanently blocks autolaunch regardless of `boot_mode`, which is the safe fallback. Application re-entry must be handled through a different target-specific mechanism (GPIO, flash flag, etc.).

```c
const uint32_t dartt_bl_get_ram_blockstart_word(void)
{
    return DARTT_BL_START_PROGRAM_KEY;
}
```
