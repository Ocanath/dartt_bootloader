# Runtime Bootloader Entry

## Current behavior
Autolaunch is not implemented. The bootloader requires an explicit `START_APPLICATION` command over DARTT to launch the application. Any system using this bootloader must have knowledge of the bootloader entry pattern before the application runs.

## Planned: RAM sentinel pattern

Reserve a single word at the top of RAM in both the bootloader and application linker scripts using a `NOLOAD` section. This value survives a software reset but is undefined after a power cycle.

**Linker scripts (both bootloader and application):**
```
MEMORY
{
  RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 32K - 4
  NOINIT (xrw) : ORIGIN = 0x20007FFC, LENGTH = 4
}

/* in SECTIONS */
.noinit (NOLOAD) :
{
    KEEP(*(.noinit))
} >NOINIT
```

**Shared declaration:**
```c
__attribute__((section(".noinit"))) volatile uint32_t bootloader_magic;
```

## Boot logic

Two conditions must both be satisfied to launch the application:
1. `fds.magic_word == APP_VALID_MAGIC` — a valid application has been flashed
2. `bootloader_magic != REENTRY_MAGIC` — no reentry request is pending

```c
if (bootloader_magic == REENTRY_MAGIC && fds.magic_word == APP_VALID_MAGIC) {
    bootloader_magic = 0;   // clear so next reset behaves normally
    // stay in bootloader
} else if (fds.magic_word == APP_VALID_MAGIC) {
    // launch app
}
// else: no valid app, stay in bootloader
```

**Application reentry sequence:**
```c
bootloader_magic = REENTRY_MAGIC;
NVIC_SystemReset();
```

Power cycle → RAM undefined → sentinel check fails → falls through to magic word check → normal boot decision.
