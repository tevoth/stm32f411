#include "sd_raw_log.h"

#include <stddef.h>
#include <string.h>

#include "sdcard_spi.h"

#define SD_RAW_LOG_SECTOR_SIZE 512U

static uint8_t log_sector[SD_RAW_LOG_SECTOR_SIZE];
static uint32_t log_sector_offset = 0U;
static uint32_t next_log_lba = SD_RAW_LOG_START_LBA;
static bool sd_available = false;

static bool sd_raw_log_flush_sector(void) {
  if (!sd_available) {
    return false;
  }

  while (log_sector_offset < SD_RAW_LOG_SECTOR_SIZE) {
    log_sector[log_sector_offset++] = '\n';
  }

  if (!sdcard_spi_write_block(next_log_lba, log_sector)) {
    sd_available = false;
    return false;
  }

  next_log_lba++;
  log_sector_offset = 0U;
  memset(log_sector, 0, sizeof(log_sector));
  return true;
}

bool sd_raw_log_init(void) {
  log_sector_offset = 0U;
  next_log_lba = SD_RAW_LOG_START_LBA;
  memset(log_sector, 0, sizeof(log_sector));

  sd_available = sdcard_spi_init();
  return sd_available;
}

bool sd_raw_log_append_line(const char *line) {
  if (!sd_available || (line == 0)) {
    return false;
  }

  size_t len = strlen(line);
  size_t i = 0U;

  while (i < len) {
    if (log_sector_offset >= SD_RAW_LOG_SECTOR_SIZE) {
      if (!sd_raw_log_flush_sector()) {
        return false;
      }
    }

    uint32_t remaining = SD_RAW_LOG_SECTOR_SIZE - log_sector_offset;
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

bool sd_raw_log_flush_pending(void) {
  if (!sd_available) {
    return false;
  }

  if (log_sector_offset == 0U) {
    return true;
  }

  return sd_raw_log_flush_sector();
}
