# cmsis_uart1

Bare-metal STM32F411 example that prints over `USART1` using CMSIS register access.

## What It Shows

- GPIO alternate-function setup for `PA9` (`USART1_TX`, AF7)
- `USART1` clock/reset/configuration
- Minimal `_write` bridge for `printf`
- Timeout-guarded transmit path

## Shared Code

This project uses shared UART helpers from:

- `shared/uart_common/inc/uart_helpers.h`

The shared helpers provide:

- `uart_wait_set_limit(...)`
- `uart_compute_bd(...)`

## Build

```bash
cmake -S . -B build
cmake --build build
```
