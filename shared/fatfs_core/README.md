# shared/fatfs_core

Shared FatFs core and disk I/O port for STM32F411 SD-card projects.

## Files

- `inc/fatfs/ff.h`
- `inc/fatfs/ffconf.h`
- `inc/fatfs/diskio.h`
- `src/ff.c`
- `src/diskio_port.c`

## Notes

- `diskio_port.c` binds FatFs block I/O to `shared/sdcard_spi`.
- This module is consumed by FatFs project variants to avoid duplicated FatFs sources.

## Current Consumers

- `cmsis_adxl_sdcard_fatfs`
- `cmsis_max6675_sdcard_fatfs`
