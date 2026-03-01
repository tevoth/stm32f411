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

- On success, firmware mounts `0:` and appends lines to `max6675_log.csv`.
- UART stays active for live monitoring.
- File data is synced periodically in `fatfs_log.c`.
