#pragma once

#include <stdbool.h>
#include <stdint.h>

// WARNING: raw-sector ring logging (no filesystem).
// Use a dedicated test card or reserve known-safe LBA ranges.
#ifndef SD_RAW_RING_START_LBA
#define SD_RAW_RING_START_LBA 32768U
#endif

#ifndef SD_RAW_RING_SECTOR_COUNT
#define SD_RAW_RING_SECTOR_COUNT 4096U
#endif

// Two metadata sectors are used for O(1) boot recovery.
#ifndef SD_RAW_RING_META_START_LBA
#define SD_RAW_RING_META_START_LBA (SD_RAW_RING_START_LBA + SD_RAW_RING_SECTOR_COUNT)
#endif

#define SD_RAW_RING_RECORD_MAGIC 0x524C4F47UL // 'RLOG'

typedef struct {
  uint32_t seq;
  uint32_t timestamp_ms;
  int32_t temp_c_x100;
  uint16_t raw;
  uint8_t status;
} sd_raw_ring_sample_t;

bool sd_raw_ring_init(void);
bool sd_raw_ring_append_sample(const sd_raw_ring_sample_t *sample);
uint32_t sd_raw_ring_next_seq(void);
uint32_t sd_raw_ring_next_lba(void);

