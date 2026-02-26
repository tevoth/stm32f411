#ifndef FATFS_LOG_H
#define FATFS_LOG_H

#include <stdbool.h>

bool fatfs_log_init(void);
bool fatfs_log_append_line(const char *line);

#endif
