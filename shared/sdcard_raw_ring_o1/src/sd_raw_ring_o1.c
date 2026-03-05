#include "sd_raw_ring_o1.h"

#include <string.h>

#include "sdcard_spi.h"

// One SD sector is 512 bytes; all on-card records are exactly one sector.
#define SD_SECTOR_SIZE 512U
// Data-record format version.
#define REC_VERSION 1U
// Metadata format version.
#define META_VERSION 1U
// Metadata magic tag ('RMET').
#define META_MAGIC 0x524D4554UL // 'RMET'
// We keep two metadata copies (A/B style) for power-loss safety.
#define META_SLOT_COUNT 2U

#pragma pack(push, 1)
typedef struct {
  // Same data-record layout as the original ring implementation.
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  uint32_t seq;
  uint32_t timestamp_ms;
  int32_t temp_c_x100;
  uint16_t raw;
  uint8_t status;
  uint8_t reserved0;
  uint32_t crc32;
  uint8_t reserved1[SD_SECTOR_SIZE - 28U];
} raw_record_t;

typedef struct {
  // Metadata says "where to write next" and "what seq comes next".
  uint32_t magic;
  uint16_t version;
  uint16_t payload_size;
  // Monotonic counter used to pick newest valid metadata copy at boot.
  uint32_t generation;
  // Write head state to restore in O(1).
  uint32_t next_lba;
  uint32_t next_seq;
  // CRC over full 512-byte metadata sector (crc32 field zeroed during calc).
  uint32_t crc32;
  uint8_t reserved[SD_SECTOR_SIZE - 24U];
} meta_record_t;
#pragma pack(pop)

_Static_assert(sizeof(raw_record_t) == SD_SECTOR_SIZE, "raw_record_t must be exactly one sector");
_Static_assert(sizeof(meta_record_t) == SD_SECTOR_SIZE, "meta_record_t must be exactly one sector");

// Shared read buffer.
static uint8_t g_sector_buf[SD_SECTOR_SIZE];
// Current runtime write head.
static uint32_t g_next_lba = SD_RAW_RING_START_LBA;
static uint32_t g_next_seq = 0U;
// Metadata bookkeeping so writes alternate between slot 0 and 1.
static uint32_t g_meta_generation = 0U;
static uint32_t g_meta_active_slot = 0U;
static bool g_meta_have_active_slot = false;
// Global ready flag (false after init/write failures).
static bool g_ready = false;

// CRC helper: feed one byte.
static uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= (uint32_t)data;
  for (uint32_t i = 0U; i < 8U; i++) {
    uint32_t mask = 0U - (crc & 1U);
    crc = (crc >> 1) ^ (0xEDB88320U & mask);
  }
  return crc;
}

// CRC helper: feed a full byte array.
static uint32_t crc32_compute(const uint8_t *data, uint32_t size) {
  uint32_t crc = 0xFFFFFFFFU;
  for (uint32_t i = 0U; i < size; i++) {
    crc = crc32_update(crc, data[i]);
  }
  return ~crc;
}

// Ring index (0..N-1) -> absolute SD LBA.
static uint32_t ring_index_to_lba(uint32_t idx) {
  return SD_RAW_RING_START_LBA + idx;
}

// Guard that metadata points into the configured ring window.
static bool lba_is_in_ring(uint32_t lba) {
  return (lba >= SD_RAW_RING_START_LBA) &&
         (lba < (SD_RAW_RING_START_LBA + SD_RAW_RING_SECTOR_COUNT));
}

// Validate one data record.
static bool record_is_valid(const raw_record_t *rec) {
  if ((rec->magic != SD_RAW_RING_RECORD_MAGIC) ||
      (rec->version != REC_VERSION) ||
      (rec->payload_size != sizeof(sd_raw_ring_sample_t))) {
    return false;
  }

  raw_record_t tmp = *rec;
  tmp.crc32 = 0U;
  uint32_t calc = crc32_compute((const uint8_t *)&tmp, sizeof(tmp));
  return calc == rec->crc32;
}

// Validate one metadata record.
static bool meta_record_is_valid(const meta_record_t *rec) {
  if ((rec->magic != META_MAGIC) ||
      (rec->version != META_VERSION) ||
      (rec->payload_size != 8U)) {
    return false;
  }
  if (!lba_is_in_ring(rec->next_lba)) {
    return false;
  }

  meta_record_t tmp = *rec;
  tmp.crc32 = 0U;
  uint32_t calc = crc32_compute((const uint8_t *)&tmp, sizeof(tmp));
  return calc == rec->crc32;
}

// Slot 0/1 -> physical metadata LBA.
static uint32_t meta_slot_to_lba(uint32_t slot) {
  return SD_RAW_RING_META_START_LBA + slot;
}

// O(1) boot path:
// Read both metadata slots, validate, pick newest generation.
static bool load_metadata(void) {
  meta_record_t meta[META_SLOT_COUNT];
  bool valid[META_SLOT_COUNT] = {false, false};

  for (uint32_t slot = 0U; slot < META_SLOT_COUNT; slot++) {
    if (!sdcard_spi_read_block(meta_slot_to_lba(slot), g_sector_buf)) {
      continue;
    }
    const meta_record_t *rec = (const meta_record_t *)g_sector_buf;
    if (!meta_record_is_valid(rec)) {
      continue;
    }
    meta[slot] = *rec;
    valid[slot] = true;
  }

  if (!valid[0] && !valid[1]) {
    return false;
  }

  // Pick newest valid generation (signed-diff handles wraparound).
  uint32_t best_slot = 0U;
  if (!valid[0]) {
    best_slot = 1U;
  } else if (valid[1] && ((int32_t)(meta[1].generation - meta[0].generation) > 0)) {
    best_slot = 1U;
  }

  g_next_lba = meta[best_slot].next_lba;
  g_next_seq = meta[best_slot].next_seq;
  g_meta_generation = meta[best_slot].generation;
  g_meta_active_slot = best_slot;
  g_meta_have_active_slot = true;
  return true;
}

// Persist current write head to alternate metadata slot.
// Writing alternate slots keeps one older valid copy if power is lost mid-write.
static bool save_metadata(void) {
  meta_record_t rec;
  memset(&rec, 0, sizeof(rec));

  rec.magic = META_MAGIC;
  rec.version = META_VERSION;
  rec.payload_size = 8U;
  rec.generation = g_meta_generation + 1U;
  rec.next_lba = g_next_lba;
  rec.next_seq = g_next_seq;
  rec.crc32 = 0U;
  rec.crc32 = crc32_compute((const uint8_t *)&rec, sizeof(rec));

  uint32_t next_slot = g_meta_have_active_slot ? (g_meta_active_slot ^ 1U) : 0U;
  if (!sdcard_spi_write_block(meta_slot_to_lba(next_slot), (const uint8_t *)&rec)) {
    return false;
  }

  g_meta_generation = rec.generation;
  g_meta_active_slot = next_slot;
  g_meta_have_active_slot = true;
  return true;
}

// Legacy/fallback recovery:
// Full ring scan to find newest valid data record.
static void choose_write_position_by_scan(void) {
  bool have_valid = false;
  uint32_t best_seq = 0U;
  uint32_t best_idx = 0U;

  for (uint32_t i = 0U; i < SD_RAW_RING_SECTOR_COUNT; i++) {
    if (!sdcard_spi_read_block(ring_index_to_lba(i), g_sector_buf)) {
      continue;
    }

    const raw_record_t *rec = (const raw_record_t *)g_sector_buf;
    if (!record_is_valid(rec)) {
      continue;
    }

    if (!have_valid || ((int32_t)(rec->seq - best_seq) > 0)) {
      have_valid = true;
      best_seq = rec->seq;
      best_idx = i;
    }
  }

  if (!have_valid) {
    g_next_lba = SD_RAW_RING_START_LBA;
    g_next_seq = 0U;
    return;
  }

  uint32_t next_idx = best_idx + 1U;
  if (next_idx >= SD_RAW_RING_SECTOR_COUNT) {
    next_idx = 0U;
  }

  g_next_lba = ring_index_to_lba(next_idx);
  g_next_seq = best_seq + 1U;
}

bool sd_raw_ring_init(void) {
  if (SD_RAW_RING_SECTOR_COUNT == 0U) {
    return false;
  }

  g_ready = sdcard_spi_init();
  if (!g_ready) {
    return false;
  }

  if (!load_metadata()) {
    // First boot / corrupted metadata / legacy card:
    // scan once, then checkpoint metadata so next boot is O(1).
    choose_write_position_by_scan();
    if (!save_metadata()) {
      g_ready = false;
      return false;
    }
  }

  return true;
}

bool sd_raw_ring_append_sample(const sd_raw_ring_sample_t *sample) {
  if (!g_ready || (sample == 0)) {
    return false;
  }

  // Build one full 512-byte data sector.
  raw_record_t rec;
  memset(&rec, 0, sizeof(rec));

  rec.magic = SD_RAW_RING_RECORD_MAGIC;
  rec.version = REC_VERSION;
  rec.payload_size = sizeof(sd_raw_ring_sample_t);
  rec.seq = sample->seq;
  rec.timestamp_ms = sample->timestamp_ms;
  rec.temp_c_x100 = sample->temp_c_x100;
  rec.raw = sample->raw;
  rec.status = sample->status;
  rec.crc32 = 0U;
  rec.crc32 = crc32_compute((const uint8_t *)&rec, sizeof(rec));

  // 1) Write data.
  if (!sdcard_spi_write_block(g_next_lba, (const uint8_t *)&rec)) {
    g_ready = false;
    return false;
  }

  // 2) Advance in-memory head.
  uint32_t idx = g_next_lba - SD_RAW_RING_START_LBA;
  idx++;
  if (idx >= SD_RAW_RING_SECTOR_COUNT) {
    idx = 0U;
  }
  g_next_lba = ring_index_to_lba(idx);
  g_next_seq = sample->seq + 1U;

  // 3) Persist new head to metadata.
  if (!save_metadata()) {
    g_ready = false;
    return false;
  }

  return true;
}

uint32_t sd_raw_ring_next_seq(void) {
  return g_next_seq;
}

uint32_t sd_raw_ring_next_lba(void) {
  return g_next_lba;
}
