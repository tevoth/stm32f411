#include "sd_raw_ring.h"

#include <string.h>

#include "sdcard_spi.h"

#define SD_SECTOR_SIZE 512U
#define REC_VERSION 1U

#pragma pack(push, 1)
typedef struct {
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
#pragma pack(pop)

_Static_assert(sizeof(raw_record_t) == SD_SECTOR_SIZE, "raw_record_t must be exactly one sector");

static uint8_t g_sector_buf[SD_SECTOR_SIZE];
static uint32_t g_next_lba = SD_RAW_RING_START_LBA;
static uint32_t g_next_seq = 0U;
static bool g_ready = false;

static uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= (uint32_t)data;
  for (uint32_t i = 0U; i < 8U; i++) {
    uint32_t mask = 0U - (crc & 1U);
    crc = (crc >> 1) ^ (0xEDB88320U & mask);
  }
  return crc;
}

static uint32_t crc32_compute(const uint8_t *data, uint32_t size) {
  uint32_t crc = 0xFFFFFFFFU;
  for (uint32_t i = 0U; i < size; i++) {
    crc = crc32_update(crc, data[i]);
  }
  return ~crc;
}

static uint32_t ring_index_to_lba(uint32_t idx) {
  return SD_RAW_RING_START_LBA + idx;
}

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

static void choose_write_position(void) {
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

  choose_write_position();
  return true;
}

bool sd_raw_ring_append_sample(const sd_raw_ring_sample_t *sample) {
  if (!g_ready || (sample == 0)) {
    return false;
  }

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

  if (!sdcard_spi_write_block(g_next_lba, (const uint8_t *)&rec)) {
    g_ready = false;
    return false;
  }

  uint32_t idx = g_next_lba - SD_RAW_RING_START_LBA;
  idx++;
  if (idx >= SD_RAW_RING_SECTOR_COUNT) {
    idx = 0U;
  }
  g_next_lba = ring_index_to_lba(idx);
  g_next_seq = sample->seq + 1U;

  return true;
}

uint32_t sd_raw_ring_next_seq(void) {
  return g_next_seq;
}

uint32_t sd_raw_ring_next_lba(void) {
  return g_next_lba;
}
