#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adxl345.h"
#include "led.h"
#include "sdcard_spi.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

// ---------- SD logging configuration ----------
// WARNING: this demo writes raw sectors, not a FAT filesystem.
// Use a dedicated test card or pick LBAs you are okay to overwrite.
#define LOG_START_LBA 32768U
#define LOG_SECTOR_SIZE 512U

// 6 bytes from ADXL345: X0 X1 Y0 Y1 Z0 Z1.
static uint8_t adxl_raw[6];

// One in-RAM sector buffer. We append text lines until full, then write 1 sector.
static uint8_t log_sector[LOG_SECTOR_SIZE];
static uint32_t log_sector_offset = 0U;
static uint32_t next_log_lba = LOG_START_LBA;

static bool sd_available = false;

static bool log_flush_sector(void) {
  if (!sd_available) {
    return false;
  }

  // Fill unused bytes with '\n' so a hex dump is easier to inspect.
  while (log_sector_offset < LOG_SECTOR_SIZE) {
    log_sector[log_sector_offset++] = '\n';
  }

  if (!sdcard_spi_write_block(next_log_lba, log_sector)) {
    return false;
  }

  next_log_lba++;
  log_sector_offset = 0U;
  memset(log_sector, 0, sizeof(log_sector));
  return true;
}

static bool log_append_line(const char *line) {
  if (!sd_available || (line == 0)) {
    return false;
  }

  size_t len = strlen(line);
  size_t i = 0U;

  // Stream line bytes into 512-byte sectors.
  while (i < len) {
    if (log_sector_offset >= LOG_SECTOR_SIZE) {
      if (!log_flush_sector()) {
        return false;
      }
    }

    uint32_t remaining = LOG_SECTOR_SIZE - log_sector_offset;
    size_t chunk = len - i;
    if (chunk > remaining) {
      chunk = remaining;
    }

    memcpy(&log_sector[log_sector_offset], &line[i], chunk);
    log_sector_offset += (uint32_t)chunk;
    i += chunk;
  }

  return true;
}

int main(void) {
  system_init();
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

  sd_available = sdcard_spi_init();
  if (sd_available) {
    printf("microSD init OK (raw log start LBA=%" PRIu32 ")\n", next_log_lba);
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
      if (sd_available) {
        if (!log_append_line(line)) {
          printf("microSD append failed; disabling SD logging\n");
          sd_available = false;
        }
      }
    }

    // Force out partial sector periodically so logs appear sooner on card.
    if (sd_available && (sample_index % 16U == 15U) && (log_sector_offset > 0U)) {
      if (!log_flush_sector()) {
        printf("microSD flush failed; disabling SD logging\n");
        sd_available = false;
      }
    }

    sample_index++;
    systick_msec_delay(100);
  }

  return 0;
}
