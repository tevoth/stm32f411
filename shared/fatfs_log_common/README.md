# shared/fatfs_log_common

Shared FatFs line-append logger used by SD-card FatFs demos.

## Files

- `inc/fatfs_log.h`
- `src/fatfs_log.c`

## Configuration

Set compile definitions per consumer project:

- `FATFS_LOG_PATH` (example: `\"0:adxl_log.csv\"`)
- `FATFS_LOG_HEADER` (example: `\"sample,ax_mg,ay_mg,az_mg\\r\\n\"`)
- Optional: `FATFS_LOG_SYNC_PERIOD` (defaults to `10`)

## Current Consumers

- `cmsis_adxl_sdcard_fatfs`
- `cmsis_max6675_sdcard_fatfs`
