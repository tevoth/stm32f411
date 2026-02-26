#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adxl345.h"
#include "fatfs/ff.h"
#include "led.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

// Small line buffer for CSV output.
#define LOG_LINE_BUF_SZ 128U

// Flush the file every N samples to reduce data loss on power failure.
#define LOG_SYNC_PERIOD 10U

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

  // -------- FatFs objects --------
  FATFS fs;
  FIL file;
  FRESULT fr;

  // Mount logical drive 0 ("0:").
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
    while (1) {
      printf("f_mount failed: %d\n", (int)fr);
      systick_msec_delay(500);
    }
  }

  // Open log file for append. Create it if it does not exist.
  fr = f_open(&file, "0:adxl_log.csv", FA_OPEN_APPEND | FA_WRITE);
  if (fr != FR_OK) {
    while (1) {
      printf("f_open failed: %d\n", (int)fr);
      systick_msec_delay(500);
    }
  }

  // If file is empty, write CSV header once.
  if (f_size(&file) == 0U) {
    const char *header = "sample,ax_mg,ay_mg,az_mg\r\n";
    UINT bw = 0U;
    fr = f_write(&file, header, (UINT)strlen(header), &bw);
    if ((fr != FR_OK) || (bw != (UINT)strlen(header))) {
      while (1) {
        printf("header write failed: %d\n", (int)fr);
        systick_msec_delay(500);
      }
    }
    (void)f_sync(&file);
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
      UINT bw = 0U;

      // UART monitoring stays active while logging to SD.
      printf("sample=%" PRIu32 " ax=%" PRId32 "mg ay=%" PRId32 "mg az=%" PRId32 "mg\n",
        sample_index, ax_mg, ay_mg, az_mg);

      fr = f_write(&file, line, (UINT)n, &bw);
      if ((fr != FR_OK) || (bw != (UINT)n)) {
        printf("f_write failed: %d\n", (int)fr);
      }

      if ((sample_index % LOG_SYNC_PERIOD) == 0U) {
        fr = f_sync(&file);
        if (fr != FR_OK) {
          printf("f_sync failed: %d\n", (int)fr);
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
