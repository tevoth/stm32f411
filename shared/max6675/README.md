# shared/max6675

Shared MAX6675 thermocouple driver for STM32F411 projects.

## Files

- `inc/max6675.h`: public API and status enum
- `src/max6675.c`: SPI read/parse implementation

## Depends On

- `shared/spi1_core`
- `shared/spi_common` (`spi_cs_xfer.h`)

## Current Consumers

- `cmsis_max6675_sdcard`
- `cmsis_max6675_sdcard_fatfs`
