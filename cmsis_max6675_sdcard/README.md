# cmsis_max6675_sdcard

Learning project for STM32F411 that:

- reads MAX6675 over **SPI1**,
- prints samples over **USART2**,
- writes text logs to a microSD card over **SPI2**.

## Pin Mapping (default in this project)

### MAX6675 (SPI1)
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

## Build

```bash
cmake -S cmsis_max6675_sdcard -B /tmp/build_cmsis_max6675_sdcard
cmake --build /tmp/build_cmsis_max6675_sdcard
```

## Important Note

This demo writes **raw SD sectors** (no FAT filesystem).
It is meant for learning SPI + SD card write flow, not for mounting in a PC filesystem as-is.
Use a dedicated test card.
