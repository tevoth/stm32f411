# shared/uart_common

Shared, transport-agnostic UART utility helpers used by UART example projects.

## Files

- `inc/uart_helpers.h`

## API

- `uart_wait_set_limit(volatile uint32_t *reg, uint32_t mask, uint32_t limit)`
  - Polls a register until `mask` bits become set or timeout expires.
  - Returns `true` on success, `false` on timeout.
- `uart_compute_bd(uint32_t periph_clk, uint32_t baudrate)`
  - Computes `BRR` divisor using nearest-integer rounding.

## Current Consumers

- `cmsis_uart1/src/uart.c`
- `cmsis_uart2/src/uart.c`
