#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "adxl345.h"
#include "fatfs_log.h"
#include "led.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

// Small line buffer for CSV output.
#define LOG_LINE_BUF_SZ 128U

static uint8_t adxl_raw[6];

int main(void) {
  if (!system_init()) {
    while (1) {
      printf("system_init failed\n");
      systick_msec_delay(500);
    }
  }
  led_init();
  uart_init();

  printf("\n=== ADXL + FAT32 logger demo ===\n");
  printf("SPI1: ADXL345, USART: console, SPI2: microSD(FAT32)\n");

  if (!adxl_init()) {
    while (1) {
      printf("ADXL345 init failed\n");
      systick_msec_delay(500);
    }
  }

  fatfs_log_file_t log_file = {0};
  bool sd_logging_enabled = fatfs_fopen(&log_file, 0);
  if (!sd_logging_enabled) {
    while (1) {
      printf("FatFs logger init failed\n");
      systick_msec_delay(500);
    }
  }

  uint32_t elapsed_ms = 0U;

  while (1) {
    led_toggle();

    if (!adxl_read(ADXL345_REG_DATA_START, adxl_raw)) {
      printf("t=%" PRIu32 "ms ADXL345 read timeout\n", elapsed_ms);
      systick_msec_delay(100);
      elapsed_ms += 100U;
      continue;
    }

    // ADXL345 outputs little-endian signed 16-bit values.
    int16_t ax = (int16_t)(((uint16_t)adxl_raw[1] << 8) | adxl_raw[0]);
    int16_t ay = (int16_t)(((uint16_t)adxl_raw[3] << 8) | adxl_raw[2]);
    int16_t az = (int16_t)(((uint16_t)adxl_raw[5] << 8) | adxl_raw[4]);

    // Convert raw LSB to milli-g using ~3.9 mg/LSB.
    int32_t ax_mg = (int32_t)ax * 39 / 10;
    int32_t ay_mg = (int32_t)ay * 39 / 10;
    int32_t az_mg = (int32_t)az * 39 / 10;

    char line[LOG_LINE_BUF_SZ];
    int n = snprintf(line, sizeof(line),
      "%" PRIu32 ",%" PRId32 ",%" PRId32 ",%" PRId32 "\r\n",
      elapsed_ms, ax_mg, ay_mg, az_mg);

    if (n > 0) {
      // UART monitoring stays active while logging to SD.
      printf("t=%" PRIu32 "ms ax=%" PRId32 "mg ay=%" PRId32 "mg az=%" PRId32 "mg\n",
        elapsed_ms, ax_mg, ay_mg, az_mg);

      if (sd_logging_enabled) {
        if (!fatfs_fprintf(&log_file, line)) {
          printf("FatFs write failed; disabling SD logging\n");
          sd_logging_enabled = false;
        }
      }
    }

    systick_msec_delay(100);
    elapsed_ms += 100U;
  }

  // Not reached in this demo.
  // f_close(&file);
  // f_mount(0, "0:", 0);
  // return 0;
}
