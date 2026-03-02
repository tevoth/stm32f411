# shared/fatfs_log_common

Shared FatFs line-append logger used by SD-card FatFs demos.

## Files

- `inc/fatfs_log.h`
- `src/fatfs_log.c`

## Configuration

Set compile definitions per consumer project:

- `FATFS_LOG_PATH` (example: `\"0:adxl_log.csv\"`)
- `FATFS_LOG_HEADER` (example: `\"time_ms,ax_mg,ay_mg,az_mg\\r\\n\"`)
- Optional: `FATFS_LOG_SYNC_PERIOD` (defaults to `10`)

## API

Primary API (handle-based):

- `bool fatfs_fopen(fatfs_log_file_t *log_file, const char *path);`
- `bool fatfs_fprintf(fatfs_log_file_t *log_file, const char *line);`

`path == NULL` uses `FATFS_LOG_PATH` from compile definitions.

Compatibility wrappers are still available:

- `fatfs_log_init()` wraps `fatfs_fopen()` using an internal default handle.
- `fatfs_log_append_line()` wraps `fatfs_fprintf()` using that default handle.

## Example

```c
fatfs_log_file_t log_file = {0};
if (!fatfs_fopen(&log_file, 0)) {
  // handle init failure
}

fatfs_fprintf(&log_file, "1000,ok,2500\r\n");
```

## Current Consumers

- `cmsis_adxl_sdcard_fatfs`
- `cmsis_max6675_sdcard_fatfs`
