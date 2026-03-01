#pragma once

#include <stdbool.h>

bool fatfs_log_init(void);
bool fatfs_log_append_line(const char *line);
