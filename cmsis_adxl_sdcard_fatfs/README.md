# cmsis_adxl_sdcard_fatfs

Learning project for STM32F411 that:

- reads ADXL345 over **SPI1**,
- prints live samples over **USART2**,
- appends CSV logs to a **FAT32 microSD card** over **SPI2**.

## Pin Mapping (default in this project)

### ADXL345 (SPI1)
- `PA5` -> SCK
- `PA6` -> MISO
- `PA7` -> MOSI
- `PA4` -> CS

### UART Console (USART2 TX)
- `PA2` -> TX

### microSD (SPI2)
- `PB13` -> SCK
- `PB14` -> MISO
- `PB15` -> MOSI
- `PB12` -> CS

## Hardware Checklist

1. Board powered and common ground shared across STM32, ADXL345, SD adapter, and UART adapter.
2. ADXL345 connected to SPI1 pins (`PA4..PA7`) and sensor power is stable.
3. microSD adapter connected to SPI2 pins (`PB12..PB15`).
4. SD card is inserted and formatted FAT32.
5. UART adapter RX connected to STM32 `PA2` (USART2 TX).
6. UART terminal set to `115200 8N1`.
7. Card level shifting is correct for your module (3.3V logic at MCU pins).

## Build

```bash
cmake -S cmsis_adxl_sdcard_fatfs -B /tmp/build_cmsis_adxl_sdcard_fatfs
cmake --build /tmp/build_cmsis_adxl_sdcard_fatfs
```

## Runtime Behavior

- On success, firmware mounts `0:` and appends lines to `adxl_log.csv`.
- UART continues printing each sample in real time.
- File data is synced periodically to reduce power-loss risk.

## Quick UART Troubleshooting

If you do not see UART output:

1. Confirm serial device path (`/dev/ttyUSB0`, `/dev/ttyACM0`, etc.).
2. Confirm terminal settings are exactly `115200 8N1`.
3. Verify wiring: STM32 `PA2` must go to adapter **RX**.
4. Verify common GND between adapter and board.
5. Reset board and reconnect terminal before rerunning.
6. If still silent, temporarily add a very early `printf("boot\n")` in `main` to separate clock/UART issues from SPI/SD issues.

Example monitor command:

```bash
stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts
stdbuf -oL cat /dev/ttyUSB0
```
