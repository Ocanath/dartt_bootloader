# DARTT Bootloader

This project aims to create a generic bootloader based on DARTT. The DARTT interface should theoretically be wire-able with relatively little effort to other microcontrollers and interfaces. 

## Basic Hardware/Peripheral Requirements

1. Access to nonvolatile storage (read and write) for reading and storing the primary device address, for interfaces such as FDCAN and serial which require it.

1. Access to the interface over which DARTT will be routed

## Functions

The bootloader will have a deferred action interface which allows the triggering of certain commands. These can include:

- Start the application
- Read memory from a specific location into the buffer
- Write the buffer to a specific memory location
- Trigger an erasure of the currently selected page(s). 
- Compute the CRC32 of the application image, for verification
- Save persistent settings
- Get the git version hash of the bootloader

Actions dispatched via the action register upon completion will load their return code into `action_status`, indicating success or failure, and indicate completion by setting `action_flag` to `NO_ACTION`. `action_status` is considered stale if `action_flag` is nonzero, and always corresponds to the most recently completed dispatched action.

### Erasure
### Writing
### Reading
### Verification


## Boundary checks

This application gets the bootloader flash region boundary via linker script. A dedicated internal variable:

``` C
extern uint32_t booltloader_end__;
```

Will be defined by the linker and contain the correct value.

**Note:** We may also embed information about the correct persistent settings location this way as well.


