# cmsis_max6675_sdcard_fatfs

Learning project for STM32F411 that:

- reads MAX6675 over **SPI1**,
- prints live samples over **USART2**,
- appends CSV logs to a **FAT32 microSD card** over **SPI2**.

## Build

```bash
cmake -S cmsis_max6675_sdcard_fatfs -B /tmp/build_cmsis_max6675_sdcard_fatfs
cmake --build /tmp/build_cmsis_max6675_sdcard_fatfs
```

## Runtime behavior

- On success, firmware mounts `0:` and appends lines to `max6675.csv`.
- UART stays active for live monitoring.
- File logging is implemented in the shared module at `shared/fatfs_log_common/src/fatfs_log.c`.
- File data is synced periodically to reduce power-loss risk.
