# Application Start Protocol


### Launch Pattern

Application launch should be supported via implementation of the stubs `dartt_bl_start_application` and `dartt_bl_cleanup_system`. 

- `dartt_bl_cleanup_system` should de-initialize all peripherals, DMA, etc and reset the system clock to default settings
- `dartt_bl_start_application` should jump to the application start address via function pointer dereference to the app start address. 

### NVIC/Interrupts
NVIC handling requires:

- `__disable_irq()` pre-jump
- Set SCP->VTOR to the application start address
- Load MSP and jump

Application code handles everything from there.

Core Concept: Bootloader just has to make sure it cleans up after itself so the Application can handle things from there.

