#include "fatfs_log.h"

#include <string.h>

#ifndef FATFS_LOG_SYNC_PERIOD
#define FATFS_LOG_SYNC_PERIOD 10U
#endif

#ifndef FATFS_LOG_PATH
#define FATFS_LOG_PATH "0:log.csv"
#endif

#ifndef FATFS_LOG_HEADER
#define FATFS_LOG_HEADER "sample,value\r\n"
#endif

static FATFS fs;
static bool fs_mounted = false;
static fatfs_log_file_t default_log;

bool fatfs_fopen(fatfs_log_file_t *log_file, const char *path) {
  if (log_file == 0) {
    return false;
  }

  if (!fs_mounted) {
    FRESULT mount_fr = f_mount(&fs, "0:", 1);
    if (mount_fr != FR_OK) {
      return false;
    }
    fs_mounted = true;
  }

  const char *log_path = (path != 0) ? path : FATFS_LOG_PATH;
  FRESULT fr = f_open(&log_file->fil, log_path, FA_OPEN_APPEND | FA_WRITE);
  if (fr != FR_OK) {
    return false;
  }

  if (f_size(&log_file->fil) == 0U) {
    const char *header = FATFS_LOG_HEADER;
    UINT bw = 0U;
    UINT expected = (UINT)strlen(header);

    fr = f_write(&log_file->fil, header, expected, &bw);
    if ((fr != FR_OK) || (bw != expected)) {
      f_close(&log_file->fil);
      return false;
    }

    if (f_sync(&log_file->fil) != FR_OK) {
      f_close(&log_file->fil);
      return false;
    }
  }

  log_file->write_count = 0U;
  log_file->ready = true;
  return true;
}

bool fatfs_fprintf(fatfs_log_file_t *log_file, const char *line) {
  if ((log_file == 0) || !log_file->ready || (line == 0)) {
    return false;
  }

  UINT bytes = (UINT)strlen(line);
  UINT bw = 0U;
  FRESULT fr = f_write(&log_file->fil, line, bytes, &bw);
  if ((fr != FR_OK) || (bw != bytes)) {
    return false;
  }

  log_file->write_count++;
  if ((log_file->write_count % FATFS_LOG_SYNC_PERIOD) == 0U) {
    if (f_sync(&log_file->fil) != FR_OK) {
      return false;
    }
  }

  return true;
}

bool fatfs_log_init(void) {
  return fatfs_fopen(&default_log, 0);
}

bool fatfs_log_append_line(const char *line) {
  return fatfs_fprintf(&default_log, line);
}
