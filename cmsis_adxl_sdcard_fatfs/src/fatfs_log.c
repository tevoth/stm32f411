#include "fatfs_log.h"

#include <stdint.h>
#include <string.h>

#include "fatfs/ff.h"

#define FATFS_LOG_SYNC_PERIOD 10U
#define FATFS_LOG_PATH "0:adxl_log.csv"

static FATFS fs;
static FIL file;
static bool log_ready = false;
static uint32_t write_count = 0U;

bool fatfs_log_init(void) {
  FRESULT fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
    return false;
  }

  fr = f_open(&file, FATFS_LOG_PATH, FA_OPEN_APPEND | FA_WRITE);
  if (fr != FR_OK) {
    return false;
  }

  if (f_size(&file) == 0U) {
    const char *header = "sample,ax_mg,ay_mg,az_mg\r\n";
    UINT bw = 0U;
    UINT expected = (UINT)strlen(header);

    fr = f_write(&file, header, expected, &bw);
    if ((fr != FR_OK) || (bw != expected)) {
      f_close(&file);
      return false;
    }

    if (f_sync(&file) != FR_OK) {
      f_close(&file);
      return false;
    }
  }

  write_count = 0U;
  log_ready = true;
  return true;
}

bool fatfs_log_append_line(const char *line) {
  if (!log_ready || (line == 0)) {
    return false;
  }

  UINT bytes = (UINT)strlen(line);
  UINT bw = 0U;
  FRESULT fr = f_write(&file, line, bytes, &bw);
  if ((fr != FR_OK) || (bw != bytes)) {
    return false;
  }

  write_count++;
  if ((write_count % FATFS_LOG_SYNC_PERIOD) == 0U) {
    if (f_sync(&file) != FR_OK) {
      return false;
    }
  }

  return true;
}
