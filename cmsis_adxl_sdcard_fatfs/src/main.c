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
  system_init();
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

  bool sd_logging_enabled = fatfs_log_init();
  if (!sd_logging_enabled) {
    while (1) {
      printf("FatFs logger init failed\n");
      systick_msec_delay(500);
    }
  }

  uint32_t sample_index = 0U;

  while (1) {
    led_toggle();

    if (!adxl_read(ADXL345_REG_DATA_START, adxl_raw)) {
      printf("ADXL345 read timeout\n");
      systick_msec_delay(100);
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
      sample_index, ax_mg, ay_mg, az_mg);

    if (n > 0) {
      // UART monitoring stays active while logging to SD.
      printf("sample=%" PRIu32 " ax=%" PRId32 "mg ay=%" PRId32 "mg az=%" PRId32 "mg\n",
        sample_index, ax_mg, ay_mg, az_mg);

      if (sd_logging_enabled) {
        if (!fatfs_log_append_line(line)) {
          printf("FatFs write failed; disabling SD logging\n");
          sd_logging_enabled = false;
        }
      }
    }

    sample_index++;
    systick_msec_delay(100);
  }

  // Not reached in this demo.
  // f_close(&file);
  // f_mount(0, "0:", 0);
  // return 0;
}
