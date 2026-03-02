#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fatfs/ff.h"

typedef struct {
  FIL fil;
  bool ready;
  uint32_t write_count;
} fatfs_log_file_t;

bool fatfs_fopen(fatfs_log_file_t *log_file, const char *path);
bool fatfs_fprintf(fatfs_log_file_t *log_file, const char *line);

bool fatfs_log_init(void);
bool fatfs_log_append_line(const char *line);
