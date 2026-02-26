#ifndef SD_RAW_LOG_H
#define SD_RAW_LOG_H

#include <stdbool.h>
#include <stdint.h>

// WARNING: raw-sector logging (no filesystem).
// Use a dedicated test card or select safe LBAs.
#define SD_RAW_LOG_START_LBA 32768U

bool sd_raw_log_init(void);
bool sd_raw_log_append_line(const char *line);
bool sd_raw_log_flush_pending(void);

#endif
