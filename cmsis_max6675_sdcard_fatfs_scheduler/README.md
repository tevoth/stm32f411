# cmsis_max6675_sdcard_fatfs_scheduler

Scheduler sketch project for STM32F411 that:

- reads MAX6675 over **SPI1**,
- prints status over **USART2**,
- appends CSV logs to a **FAT32 microSD card** over **SPI2**,
- schedules each activity at different periods in a cooperative main loop.

## Build

```bash
cmake -S cmsis_max6675_sdcard_fatfs_scheduler -B /tmp/build_cmsis_max6675_sdcard_fatfs_scheduler
cmake --build /tmp/build_cmsis_max6675_sdcard_fatfs_scheduler
```

## Scheduler model

- `SysTick_Handler` increments a 1 ms counter.
- Main loop checks a task table:
  - `sample` every 250 ms
  - `uart` every 500 ms
  - `sd` every 1000 ms
  - `stats` every 5000 ms
- ISR only provides timing. Sensor reads, UART output, and FATFS writes run in thread/main context.

## Runtime stats on UART

The `stats` task prints per-task scheduler health:

- `run`: total dispatches
- `late`: number of times dispatch started after its due time
- `skip`: estimated number of full periods missed (`lateness / period`)
- `maxLate`: worst observed start lateness in ms
- `maxRun`: worst observed execution time in ms

## Notes

- This is a cooperative scheduler (no RTOS).
- Add a new periodic job by creating a new `task_*()` function and appending a row in the `g_tasks[]` table in `src/main.c`.
- SD logging auto-disables if append fails; UART output continues.
