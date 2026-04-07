# Bare-Metal Init Checklist (STM32G431CBTx Bootloader)

Replacing `MX_USART2_UART_Init` + `SystemClock_Config` with direct register writes.
Target savings: ~5.1KB off release build (16.95KB → ~11.85KB floor).

---

## SystemClock_Config replacement

- [ ] Enable HSE bypass and wait for HSERDY (`RCC->CR`)
- [ ] Enable PWR clock (`RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN`)
- [ ] Set voltage scale Range 1 Boost: clear `PWR_CR5_R1MODE` in `PWR->CR5`
- [ ] Set flash latency to 4 WS + enable ICEN, DCEN, PRFTEN (`FLASH->ACR`), poll until latency bits confirm
- [ ] Configure PLL via `RCC->PLLCFGR`: src=HSE, M=12 (stored as 11), N=85, P=2 (stored as 1), Q=2, R=2 — enable PLLPEN, PLLQEN, PLLREN
  - **Verify PLLQ/PLLR encoding against RM0440 Table 56** — encoding differs from PLLM/PLLP
- [ ] Enable PLL (`RCC->CR |= RCC_CR_PLLON`), wait for PLLRDY
- [ ] Switch SYSCLK to PLL (`RCC->CFGR`), AHB/APB1/APB2 all div1 (bits = 0), wait for SWS confirms
- [ ] Set `SystemCoreClock = 170000000UL`
- [ ] Remove `HAL_PWR_MODULE_ENABLED` and `HAL_RCC_MODULE_ENABLED` from `stm32g4xx_hal_conf.h` once confirmed working

---

## USART2 + DMA init replacement

Replaces `MX_USART2_UART_Init` and the relevant parts of `HAL_UART_MspInit`.
`m_uart_start_interrupts` is kept as-is — it handles DMA channel register programming and UART CR1/CR3 setup.

### HAL_MspInit equivalent
- [ ] `RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN`
- [ ] `RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN`
- [ ] Disable UCPD dead battery: `PWR->CR3 |= PWR_CR3_UCPD_DBDIS`

### RCC peripheral clocks
- [ ] `RCC->AHB1ENR |= RCC_AHB1ENR_DMAMUXEN | RCC_AHB1ENR_DMA1EN` (may already be in MX_DMA_Init — verify before removing that call)
- [ ] `RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN`
- [ ] Verify USART2 clock source = PCLK1: `RCC->CCIPR` bits [3:2] should be 00 (default on reset, may not need explicit write)

### GPIO — PA2 (TX, AF7) and PA3 (RX, AF7)
GPIOA clock already enabled in `MX_GPIO_Init` — confirm PA2/PA3 are not clobbered there.
- [ ] MODER: PA2 and PA3 → alternate function mode (0b10)
- [ ] AFRL: PA2 → AF7, PA3 → AF7
- [ ] OTYPER: push-pull (0)
- [ ] OSPEEDR: low speed (0b00)
- [ ] PUPDR: no pull (0b00)

### USART2 registers
- [ ] BRR: set for 921600 baud @ PCLK1 (170MHz / 921600 ≈ 184 → `USART2->BRR = 184`)
- [ ] CR1: 8-bit word, no parity, oversampling-by-16 (all default) — just enable UE (`USART_CR1_UE`)
- [ ] CR2: 1 stop bit (default, no write needed)
- [ ] CR3: no hardware flow control (default)
- [ ] `m_uart_start_interrupts(&m_huart2)` handles RE, TE, RXNEIE, DMAR, DMAT, and all DMA channel registers

### DMAMUX routing
**Ground truth is the `m_huart2` struct** (`rxdma = DMA1_Channel2`, `txdma = DMA1_Channel3`).
The HAL MspInit had this wrong (was using Channel1/Channel2) — bare-metal replacement must use Channel2/Channel3.

- [ ] Route USART2_RX (request ID 27) → DMA1_Channel2: `DMAMUX1_Channel1->CCR = 27` (DMAMUX channel index is DMA channel - 1)
- [ ] Route USART2_TX (request ID 28) → DMA1_Channel3: `DMAMUX1_Channel2->CCR = 28`
- [ ] Verify request IDs against RM0440 Table 91

### NVIC
- [ ] `NVIC_SetPriority(USART2_IRQn, 0)`
- [ ] `NVIC_EnableIRQ(USART2_IRQn)`
- [ ] DMA1 Channel2 and Channel3 IRQs: priority + enable (currently done via HAL_NVIC in `MX_DMA_Init` — keep or inline)

---

## Cleanup after both are working
- [ ] Remove `HAL_UART_MODULE_ENABLED` from `stm32g4xx_hal_conf.h`
- [ ] Remove `stm32g4xx_hal_msp.c` or gut it down to the MspInit stub only
- [ ] Assess whether `HAL_Init()` can be replaced with just the MspInit equivalent above + `NVIC_SetPriorityGrouping`
- [ ] Remove `sysmem.c` and `syscalls.c` if nothing pulls in newlib heap/stdio (verify with map file)
