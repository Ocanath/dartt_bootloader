# DARTT Bootloader Flashing Tool

## Introduction

The DARTT UART/RS485 flashing tool is a command-line DFU tool for updating the firmware of DARTT bootloader compatible devices. 

It accepts the following file types:
- `.bin`
- `.elf`

Support for `.elf` is provided by the `ELFIO` dependency.

Example call pattern:

```bash
dartt_flash 255 application.elf		# Flash a device running a dartt bootloader with address 255
```

All bootloader operations require a mandatory address argument corresponding to the main/primary DARTT address. **DARTT bootloaders have a target-specific default address on startup.** On STM32, that means a primary address of 255, misc address of 0. 

To retrieve the list of available commands, call the tool with:

```bash
dartt_flash -h
```

## Flashing

The flashing procedure works as follows:

1. Load the application start pointer from the target
1. If the input file is an .elf, compare the application start address to the file start address - if there is a mismatch, throw an error. To skip the application start address checks for .elf file, you may call the tool with the `--no-application` argument. This is useful for things like managing persistent flash settings, etc. Flashing binaries with the `--no-application` flag will trigger a full read-back verification of the target region (as opposed to CRC32 verification), unless the `--no-verify` option is also specified.
1. Retrieve the attributes section, particularly `.page_size` and `.write_size`
1. Check page alignment of the first address and throw an error if the start address isn't page aligned. This is a critical error indicating a misconfigured target.
1. Begin writing the blob. Erase the first page, fill the write buffer, and dispatch flash write operations until the page fills, then continue to the next page, for each page which must contain new memory. Pages which do not need to be modified remain untouched by the tool.
1. Once writing is complete, dispatch the bootloader's crc32 calculation and compare it to an internally computed crc32. Indicate match/success or mismatch/failure.
1. If success, write the application size and trigger a settings update, then exit if successful. The `--skip-save` flag can be used to force the tool to skip updating the bootloader persistent settings section. `--no-verify` and `--no-application` also implicitly set `--skip-save`. 

Binary (.bin) files are always written starting at the retrieved application start address, unless a `--start` argument is specified. `--start` is invalid for .elf files and will cause the program to exit with an error.

Example usage of `--start`:

```bash
dartt_flash 22 settings.bin --start 0x0801F800
```

The flashing tool will pre-check whether the binary fits using 

## Erasing

The tool can mass-erase the target using the `--eraseall` flag. This erases everything starting at the application start address to the final page of memory. The number of page erasures which are dispatched and their index are calculated by fetching the app pointer and using the `.num_pages` attribute. The bootloader region is protected - attempts to erase it should return a blocked error, unless something is badly misconfigured on the target hardware implementation.

## Verification

By default, all files will be verified via CRC32 when flashed. If you wish to flash without verification, you must use the `--no-verify` option, i.e.:

`dartt_flash 10 application.elf --no-verify`

If the `--verify` option is used, only verification is performed. Example:

```bash
dartt_flash 7 application.elf --verify
```

This performs only the verification step, skipping flashing, and indicates whether there is a match on exit.

## Launching

The program can be launched via the flashing tool via the `--launch` command. Example:

```bash
dartt_flash 17 --launch
```

This simply triggers the `START_APPLICATION` command and exits the program.

## Reading

The flashing tool can also be used to read out memory from the device. This is triggered with the `-o` command followed by a destination filename argument (must have type `.bin` or `.elf`). By default, the entire memory is read out. 

Reading may also be performed at specific regions using `--rstart` and `--rlen`. Example:

```bash
dartt_flash 10 --rstart 0x0801F800 --rlen 0x800 -o settings.bin
```

Where `--rstart` is an absolute address and `--rlen` is in bytes. 

If written out to a `.elf`, the absolute address in memory is used. The entire available flash region is accessible, including the bootloader itself, if desired.

## Version

You can retrieve the bootloader version using the `--version` flag:

```bash
dartt_flash 10 --version
```

This dispatches the action of loading the working buffer with the git version, which the flashing tool retrieves and prints once finished polling the action registers. The tool will return immediately after obtaining the version.

## Baud Rate

The default baudrate is 921600. To specify a new baudrate, use the `--baudrate` argument, ex.

```bash
dartt_flash 10 program.elf --baudrate 230400
```

---
## Implementation Notes

The flashing tool uses `dartt_sync`, `dartt_read_multi`, and `dartt_write_multi` with `serial-cross-platform` based callbacks for all dartt operations.

Dispatched actions have their own wrapper functions to set the flag and poll status. Every dispatched action must be followed up by polling the flag and status registers until they clear. If a dispatched action throws an error, the program will exit with a code and warning message.

The tool links to the `shared` bootloader core for control struct definition and helper functions.


ELFIO will be used for `.elf` file parsing and writing. 