# DARTT Bootloader

This project aims to create a generic bootloader based on DARTT. The DARTT interface should theoretically be wire-able with relatively little effort to other microcontrollers and interfaces. 

## Basic Hardware/Peripheral Requirements

1. Access to nonvolatile storage (read and write) for reading and storing the primary device address, for interfaces such as FDCAN and serial which require it.

1. Access to the interface over which DARTT will be routed

## Functions

1. The bootloader will have a deferred action interface which allows the triggering of certain commands. These can include:

1. Read memory from a specific location into the buffer
1. Trigger an erasure of the currently selected page(s). 
1. Write the buffer to a specific memory location
1. Compute the CRC32 of the application image, for verification

