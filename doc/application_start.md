# Application Start Protocol


### Launch Pattern

- Magic word: flag startup word to jump to application, restart. Startup jump check happens before peripherals get dirty so things are set up properly. Main tradeoff is that you can soft-brick if the application gets locked out of access to the magic word.

- De-init everything: Risky. Application defined, multiple applications. Might be able to leverage reset control register to make it simpler? TBD

### NVIC/Interrupts
NVIC handling requires:

- `__disable_irq()` pre-jump
- Set SCP->VTOR to the application start address
- Load MSP and jump

Application code handles everything from there.

Core Concept: Bootloader just has to make sure it cleans up after itself so the Application can handle things from there.

