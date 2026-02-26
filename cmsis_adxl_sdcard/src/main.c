#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "adxl345.h"
#include "led.h"
#include "sd_raw_log.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

// 6 bytes from ADXL345: X0 X1 Y0 Y1 Z0 Z1.
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

  printf("\n=== ADXL + microSD logger demo ===\n");
  printf("SPI1: ADXL345, USART: console, SPI2: microSD\n");

  if (!adxl_init()) {
    while (1) {
      printf("ADXL345 init failed\n");
      systick_msec_delay(500);
    }
  }

  bool sd_logging_enabled = sd_raw_log_init();
  if (sd_logging_enabled) {
    printf("microSD init OK (raw log start LBA=%" PRIu32 ")\n", (uint32_t)SD_RAW_LOG_START_LBA);
  } else {
    printf("microSD init FAILED, continuing with UART only\n");
  }

  uint32_t sample_index = 0U;

  while (1) {
    led_toggle();

    if (!adxl_read(ADXL345_REG_DATA_START, adxl_raw)) {
      printf("ADXL345 read timeout\n");
      systick_msec_delay(100);
      continue;
    }

    // Rebuild signed 16-bit values from little-endian register pairs.
    int16_t ax = (int16_t)(((uint16_t)adxl_raw[1] << 8) | adxl_raw[0]);
    int16_t ay = (int16_t)(((uint16_t)adxl_raw[3] << 8) | adxl_raw[2]);
    int16_t az = (int16_t)(((uint16_t)adxl_raw[5] << 8) | adxl_raw[4]);

    // Convert raw counts to milli-g with the 3.9 mg/LSB approximation.
    int32_t ax_mg = (int32_t)ax * 39 / 10;
    int32_t ay_mg = (int32_t)ay * 39 / 10;
    int32_t az_mg = (int32_t)az * 39 / 10;

    char line[128];
    int n = snprintf(line, sizeof(line),
      "sample=%" PRIu32 ", ax=%" PRId32 "mg, ay=%" PRId32 "mg, az=%" PRId32 "mg\n",
      sample_index, ax_mg, ay_mg, az_mg);

    if (n > 0) {
      // Always print to UART so you can monitor live behavior.
      printf("%s", line);

      // Also append to SD log when card is available.
      if (sd_logging_enabled) {
        if (!sd_raw_log_append_line(line)) {
          printf("microSD append failed; disabling SD logging\n");
          sd_logging_enabled = false;
        }
      }
    }

    // Force out partial sector periodically so logs appear sooner on card.
    if (sd_logging_enabled && (sample_index % 16U == 15U)) {
      if (!sd_raw_log_flush_pending()) {
        printf("microSD flush failed; disabling SD logging\n");
        sd_logging_enabled = false;
      }
    }

    sample_index++;
    systick_msec_delay(100);
  }

  return 0;
}
