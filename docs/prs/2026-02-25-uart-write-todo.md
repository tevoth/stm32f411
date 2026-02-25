# PR: Track `_write` error-handling follow-up in UART modules

## Summary
This change does **not** alter runtime behavior.

It adds TODO comments in all UART `_write()` implementations where invalid input (`ptr == 0 && len > 0`) currently returns `0`.

## Why
Returning `0` can hide an invalid buffer condition from callers. We are intentionally deferring the behavioral change for now and documenting it at the call site.

## Files Changed
- `cmsis_uart1/src/uart.c`
- `cmsis_uart2/src/uart.c`
- `cmsis_spi/src/uart.c`
- `cmsis_dac/src/uart.c`
- `cmsis_max6675/src/uart.c`

## Added TODO
`// TODO: return -1 (and optionally set errno) for invalid buffer input.`

## Verification
- Built affected targets successfully:
  - `cmsis_uart1`
  - `cmsis_uart2`
  - `cmsis_spi`
  - `cmsis_dac`
  - `cmsis_max6675`
