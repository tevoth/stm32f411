#include "sd_raw_ring.h"

#include <string.h>

#include "sdcard_spi.h"

// A sector is one SD card "box" we can read/write at once.
#define SD_SECTOR_SIZE 512U
// Version number for our record shape.
#define REC_VERSION 1U

#pragma pack(push, 1)
typedef struct {
  // "Magic" is a secret label saying "this is one of our records".
  uint32_t magic;
  // Record format version.
  uint16_t version;
  // How big the useful payload is.
  uint16_t payload_size;

  // Counting number that grows each sample (0,1,2,3...).
  uint32_t seq;
  // Time when sample was taken.
  uint32_t timestamp_ms;
  // Temperature * 100 (so 2534 means 25.34 C).
  int32_t temp_c_x100;
  // Raw sensor value.
  uint16_t raw;
  // Status bits.
  uint8_t status;
  // Unused byte for alignment/space.
  uint8_t reserved0;

  // Checksum so we can detect broken/corrupt data.
  uint32_t crc32;
  // Fill the rest so the struct is exactly one sector.
  uint8_t reserved1[SD_SECTOR_SIZE - 28U];
} raw_record_t;
#pragma pack(pop)

// Compile-time guard: record must be exactly 512 bytes.
_Static_assert(sizeof(raw_record_t) == SD_SECTOR_SIZE, "raw_record_t must be exactly one sector");

// Shared 512-byte buffer for reads.
static uint8_t g_sector_buf[SD_SECTOR_SIZE];
// Next SD address (LBA) to write to.
static uint32_t g_next_lba = SD_RAW_RING_START_LBA;
// Next sequence number expected.
static uint32_t g_next_seq = 0U;
// True after init works; false if not ready or write failed.
static bool g_ready = false;

// CRC helper: feed one byte into current CRC.
static uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= (uint32_t)data;
  for (uint32_t i = 0U; i < 8U; i++) {
    uint32_t mask = 0U - (crc & 1U);
    crc = (crc >> 1) ^ (0xEDB88320U & mask);
  }
  return crc;
}

// CRC over a whole byte array.
static uint32_t crc32_compute(const uint8_t *data, uint32_t size) {
  uint32_t crc = 0xFFFFFFFFU;
  for (uint32_t i = 0U; i < size; i++) {
    crc = crc32_update(crc, data[i]);
  }
  return ~crc;
}

// Convert ring index (0..N-1) into real SD LBA address.
static uint32_t ring_index_to_lba(uint32_t idx) {
  return SD_RAW_RING_START_LBA + idx;
}

// Check if a record looks like ours and checksum matches.
static bool record_is_valid(const raw_record_t *rec) {
  if ((rec->magic != SD_RAW_RING_RECORD_MAGIC) ||
      (rec->version != REC_VERSION) ||
      (rec->payload_size != sizeof(sd_raw_ring_sample_t))) {
    return false;
  }

  // Copy record, zero CRC field, recompute CRC, compare.
  raw_record_t tmp = *rec;
  tmp.crc32 = 0U;
  uint32_t calc = crc32_compute((const uint8_t *)&tmp, sizeof(tmp));
  return calc == rec->crc32;
}

// On boot: scan whole ring and find newest valid record.
// Then choose the slot right after it as next write place.
static void choose_write_position(void) {
  bool have_valid = false;
  uint32_t best_seq = 0U;
  uint32_t best_idx = 0U;

  for (uint32_t i = 0U; i < SD_RAW_RING_SECTOR_COUNT; i++) {
    if (!sdcard_spi_read_block(ring_index_to_lba(i), g_sector_buf)) {
      // Read failed, skip this slot.
      continue;
    }

    const raw_record_t *rec = (const raw_record_t *)g_sector_buf;
    if (!record_is_valid(rec)) {
      // Not our record or corrupted.
      continue;
    }

    // Keep the largest/newest seq found so far.
    if (!have_valid || ((int32_t)(rec->seq - best_seq) > 0)) {
      have_valid = true;
      best_seq = rec->seq;
      best_idx = i;
    }
  }

  if (!have_valid) {
    // Card looks empty for this ring area.
    g_next_lba = SD_RAW_RING_START_LBA;
    g_next_seq = 0U;
    return;
  }

  // Next write is one slot after newest record, wrapping around.
  uint32_t next_idx = best_idx + 1U;
  if (next_idx >= SD_RAW_RING_SECTOR_COUNT) {
    next_idx = 0U;
  }

  g_next_lba = ring_index_to_lba(next_idx);
  g_next_seq = best_seq + 1U;
}

// Start SD system and recover next write position.
bool sd_raw_ring_init(void) {
  if (SD_RAW_RING_SECTOR_COUNT == 0U) {
    return false;
  }

  g_ready = sdcard_spi_init();
  if (!g_ready) {
    return false;
  }

  choose_write_position();
  return true;
}

// Add one sample to ring log.
bool sd_raw_ring_append_sample(const sd_raw_ring_sample_t *sample) {
  if (!g_ready || (sample == 0)) {
    return false;
  }

  raw_record_t rec;
  memset(&rec, 0, sizeof(rec));

  // Fill record header + payload.
  rec.magic = SD_RAW_RING_RECORD_MAGIC;
  rec.version = REC_VERSION;
  rec.payload_size = sizeof(sd_raw_ring_sample_t);
  rec.seq = sample->seq;
  rec.timestamp_ms = sample->timestamp_ms;
  rec.temp_c_x100 = sample->temp_c_x100;
  rec.raw = sample->raw;
  rec.status = sample->status;

  // Compute checksum after zeroing crc field.
  rec.crc32 = 0U;
  rec.crc32 = crc32_compute((const uint8_t *)&rec, sizeof(rec));

  // Write one full sector to SD.
  if (!sdcard_spi_write_block(g_next_lba, (const uint8_t *)&rec)) {
    // If write fails, mark not-ready so caller can re-init.
    g_ready = false;
    return false;
  }

  // Move write head to next slot in ring (wrap if needed).
  uint32_t idx = g_next_lba - SD_RAW_RING_START_LBA;
  idx++;
  if (idx >= SD_RAW_RING_SECTOR_COUNT) {
    idx = 0U;
  }
  g_next_lba = ring_index_to_lba(idx);
  g_next_seq = sample->seq + 1U;

  return true;
}

// Tell caller what seq should come next.
uint32_t sd_raw_ring_next_seq(void) {
  return g_next_seq;
}

// Tell caller what LBA will be written next.
uint32_t sd_raw_ring_next_lba(void) {
  return g_next_lba;
}
