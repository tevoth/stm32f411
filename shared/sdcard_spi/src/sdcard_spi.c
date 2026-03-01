#include "sdcard_spi.h"

#include "system_clock.h"
#include "spi2_sd.h"

#define SDCARD_BLOCK_SIZE 512U

// Standard SD commands in SPI mode.
#define CMD0   0U   // GO_IDLE_STATE: reset card to idle in SPI mode
#define CMD8   8U   // SEND_IF_COND: check voltage range and SD v2 support
#define CMD9   9U   // SEND_CSD: read card-specific data register
#define CMD16 16U   // SET_BLOCKLEN: set block length (used for SDSC cards)
#define CMD17 17U   // READ_SINGLE_BLOCK: read one 512-byte data block
#define CMD24 24U   // WRITE_BLOCK: write one 512-byte data block
#define CMD55 55U   // APP_CMD prefix: next command is application-specific
#define CMD58 58U   // READ_OCR: read OCR/capacity status bits

#define ACMD41 41U  // SD_SEND_OP_COND: complete card initialization
#define ACMD41_ARG_HCS 0x40000000U // Host supports SDHC/SDXC block addressing

// Data token for single block write in SPI mode.
#define TOKEN_SINGLE_BLOCK_WRITE 0xFEU
#define TOKEN_SINGLE_BLOCK_READ  0xFEU

// Data response token low bits after write.
#define DATA_RESP_ACCEPTED 0x05U

static bool sd_high_capacity = false;
static uint32_t sd_sector_count = 0U;

static bool spi_txrx(uint8_t tx, uint8_t *rx) {
  return spi2_sd_transfer(tx, rx);
}

static bool sd_send_idle_clocks(uint32_t count) {
  uint8_t rx = 0U;
  for (uint32_t i = 0; i < count; i++) {
    if (!spi_txrx(0xFFU, &rx)) {
      return false;
    }
  }
  return true;
}

static bool sd_wait_not_busy(uint32_t max_bytes) {
  // Card drives MISO low while internally busy. Ready state is 0xFF.
  uint8_t rx = 0U;
  for (uint32_t i = 0; i < max_bytes; i++) {
    if (!spi_txrx(0xFFU, &rx)) {
      return false;
    }
    if (rx == 0xFFU) {
      return true;
    }
  }
  return false;
}

static bool sd_wait_r1(uint8_t *r1, uint32_t max_bytes) {
  // R1 response has bit7 cleared when valid.
  uint8_t rx = 0xFFU;
  for (uint32_t i = 0; i < max_bytes; i++) {
    if (!spi_txrx(0xFFU, &rx)) {
      return false;
    }
    if ((rx & 0x80U) == 0U) {
      *r1 = rx;
      return true;
    }
  }
  return false;
}

static bool sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *r1) {
  uint8_t rx = 0U;

  // Command packet is always 6 bytes in SPI mode.
  uint8_t pkt[6];
  pkt[0] = (uint8_t)(0x40U | cmd);
  pkt[1] = (uint8_t)(arg >> 24);
  pkt[2] = (uint8_t)(arg >> 16);
  pkt[3] = (uint8_t)(arg >> 8);
  pkt[4] = (uint8_t)(arg >> 0);
  pkt[5] = crc;

  // At least one idle byte before command.
  if (!spi_txrx(0xFFU, &rx)) {
    return false;
  }

  for (uint32_t i = 0; i < 6U; i++) {
    if (!spi_txrx(pkt[i], &rx)) {
      return false;
    }
  }

  return sd_wait_r1(r1, 16U);
}

static bool sd_send_acmd(uint8_t acmd, uint32_t arg, uint8_t *r1) {
  uint8_t resp = 0xFFU;

  // Application command is prefixed by CMD55.
  if (!sd_send_cmd(CMD55, 0U, 0x01U, &resp)) {
    return false;
  }
  if ((resp & 0xFEU) != 0U) {
    return false;
  }

  if (!sd_send_cmd(acmd, arg, 0x01U, &resp)) {
    return false;
  }

  *r1 = resp;
  return true;
}

static bool sd_read_data_block(uint8_t start_token, uint8_t *data, uint32_t size, uint32_t max_wait) {
  uint8_t rx = 0xFFU;

  for (uint32_t i = 0; i < max_wait; i++) {
    if (!spi_txrx(0xFFU, &rx)) {
      return false;
    }
    if (rx == start_token) {
      break;
    }
  }
  if (rx != start_token) {
    return false;
  }

  for (uint32_t i = 0; i < size; i++) {
    if (!spi_txrx(0xFFU, &data[i])) {
      return false;
    }
  }

  // Ignore CRC bytes in SPI mode when CRC is disabled.
  if (!spi_txrx(0xFFU, &rx) || !spi_txrx(0xFFU, &rx)) {
    return false;
  }

  return true;
}

static bool sd_parse_sector_count_from_csd(const uint8_t csd[16], uint32_t *sector_count) {
  if ((csd == 0) || (sector_count == 0)) {
    return false;
  }

  uint8_t csd_structure = (uint8_t)((csd[0] >> 6) & 0x03U);
  if (csd_structure == 1U) {
    // CSD v2.0: memory capacity = (C_SIZE + 1) * 512 KiB.
    uint32_t c_size = ((uint32_t)(csd[7] & 0x3FU) << 16) |
                      ((uint32_t)csd[8] << 8) |
                      (uint32_t)csd[9];
    *sector_count = (c_size + 1U) * 1024U;
    return (*sector_count > 0U);
  }

  if (csd_structure == 0U) {
    // CSD v1.0 (SDSC): compute byte capacity from C_SIZE/C_SIZE_MULT/READ_BL_LEN.
    uint32_t read_bl_len = (uint32_t)(csd[5] & 0x0FU);
    uint32_t c_size = ((uint32_t)(csd[6] & 0x03U) << 10) |
                      ((uint32_t)csd[7] << 2) |
                      ((uint32_t)(csd[8] & 0xC0U) >> 6);
    uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03U) << 1) |
                           ((uint32_t)(csd[10] & 0x80U) >> 7);

    uint64_t block_len = (uint64_t)1U << read_bl_len;
    uint64_t block_count = (uint64_t)(c_size + 1U) << (c_size_mult + 2U);
    uint64_t capacity_bytes = block_len * block_count;
    uint64_t sectors = capacity_bytes / SDCARD_BLOCK_SIZE;

    if ((sectors == 0U) || (sectors > 0xFFFFFFFFULL)) {
      return false;
    }

    *sector_count = (uint32_t)sectors;
    return true;
  }

  return false;
}

bool sdcard_spi_init(void) {
  uint8_t r1 = 0xFFU;
  uint8_t rx = 0U;
  uint8_t csd[16];
  sd_high_capacity = false;
  sd_sector_count = 0U;

  spi2_sd_init();
  spi2_sd_set_slow_clock();

  // Keep CS high while sending startup clocks to enter SPI mode.
  spi2_sd_cs_high();
  if (!sd_send_idle_clocks(20U)) {
    return false;
  }

  spi2_sd_cs_low();

  // CMD0 puts the card into idle state.
  if (!sd_send_cmd(CMD0, 0U, 0x95U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x01U) {
    spi2_sd_cs_high();
    return false;
  }

  // CMD8 checks voltage range and v2 support.
  if (!sd_send_cmd(CMD8, 0x000001AAU, 0x87U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x01U) {
    // This starter code expects SD v2 cards.
    spi2_sd_cs_high();
    return false;
  }

  // Read the rest of R7 (4 bytes) and validate check pattern 0x1AA.
  uint8_t r7[4];
  for (uint32_t i = 0; i < 4U; i++) {
    if (!spi_txrx(0xFFU, &r7[i])) {
      spi2_sd_cs_high();
      return false;
    }
  }
  if ((r7[2] != 0x01U) || (r7[3] != 0xAAU)) {
    spi2_sd_cs_high();
    return false;
  }

  // Repeatedly issue ACMD41 until the card leaves idle state.
  // Use a SysTick-based 2000 ms wall-clock timeout so the retry count is
  // independent of the SPI clock speed and NCR wait bytes, which both vary.
  // Deassert CS between retries: many cards need the rising CS edge to reset
  // their command-state machine before accepting the next CMD55+ACMD41 pair.
#define ACMD41_TIMEOUT_MS  2000U
#define ACMD41_TIMEOUT_CYC (ACMD41_TIMEOUT_MS * (SYSTEM_SYSCLK_HZ / 1000U))

  // Use the DWT cycle counter for a wall-clock timeout.
  // CYCCNT is never reset — we only capture a start value and compare the
  // delta — so a caller's ongoing CYCCNT usage is not disturbed.
  // SysTick is not touched at all.
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  bool dwt_was_enabled = ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U);
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  uint32_t t0 = DWT->CYCCNT;

  while ((DWT->CYCCNT - t0) < ACMD41_TIMEOUT_CYC) {
    spi2_sd_cs_high();
    if (!spi_txrx(0xFFU, &rx)) {
      spi2_sd_cs_high();
      break;
    }
    spi2_sd_cs_low();

    // After CMD8 success (SD v2), set HCS so cards can exit IDLE correctly.
    if (!sd_send_acmd(ACMD41, ACMD41_ARG_HCS, &r1)) {
      spi2_sd_cs_high();
      break;
    }
    if (r1 == 0x00U) {
      break;
    }
  }

  if (!dwt_was_enabled) {
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
  }

  if (r1 != 0x00U) {
    spi2_sd_cs_high();
    return false;
  }

  // CMD58 reads OCR to detect SDHC/SDXC addressing mode.
  if (!sd_send_cmd(CMD58, 0U, 0x01U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x00U) {
    spi2_sd_cs_high();
    return false;
  }

  uint8_t ocr[4];
  for (uint32_t i = 0; i < 4U; i++) {
    if (!spi_txrx(0xFFU, &ocr[i])) {
      spi2_sd_cs_high();
      return false;
    }
  }
  sd_high_capacity = ((ocr[0] & 0x40U) != 0U);

  // SDSC uses byte addressing and needs block length set to 512 bytes.
  if (!sd_high_capacity) {
    if (!sd_send_cmd(CMD16, SDCARD_BLOCK_SIZE, 0x01U, &r1)) {
      spi2_sd_cs_high();
      return false;
    }
    if (r1 != 0x00U) {
      spi2_sd_cs_high();
      return false;
    }
  }

  // Read CSD to determine card capacity in 512-byte sectors.
  if (!sd_send_cmd(CMD9, 0U, 0x01U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x00U) {
    spi2_sd_cs_high();
    return false;
  }
  if (!sd_read_data_block(TOKEN_SINGLE_BLOCK_READ, csd, sizeof(csd), 100000U)) {
    spi2_sd_cs_high();
    return false;
  }
  if (!sd_parse_sector_count_from_csd(csd, &sd_sector_count)) {
    spi2_sd_cs_high();
    return false;
  }

  // Release card and give it one extra idle byte after command phase.
  spi2_sd_cs_high();
  if (!spi_txrx(0xFFU, &rx)) {
    return false;
  }

  // Now we can speed up SPI for normal operation.
  spi2_sd_set_fast_clock();

  return true;
}

bool sdcard_spi_write_block(uint32_t lba, const uint8_t *data_512) {
  if (data_512 == 0) {
    return false;
  }

  uint8_t r1 = 0xFFU;
  uint8_t rx = 0U;

  // SDHC/SDXC uses block addressing, SDSC uses byte addressing.
  uint32_t address = lba;
  if (!sd_high_capacity) {
    address = lba * SDCARD_BLOCK_SIZE;
  }

  spi2_sd_cs_low();

  if (!sd_send_cmd(CMD24, address, 0x01U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x00U) {
    spi2_sd_cs_high();
    return false;
  }

  // One stuff byte before data token (common SPI write sequence).
  if (!spi_txrx(0xFFU, &rx)) {
    spi2_sd_cs_high();
    return false;
  }

  if (!spi_txrx(TOKEN_SINGLE_BLOCK_WRITE, &rx)) {
    spi2_sd_cs_high();
    return false;
  }

  for (uint32_t i = 0; i < SDCARD_BLOCK_SIZE; i++) {
    if (!spi_txrx(data_512[i], &rx)) {
      spi2_sd_cs_high();
      return false;
    }
  }

  // Dummy CRC in SPI mode unless CRC checking is explicitly enabled.
  if (!spi_txrx(0xFFU, &rx) || !spi_txrx(0xFFU, &rx)) {
    spi2_sd_cs_high();
    return false;
  }

  // Data response token: xxx0AAA1, AAA=010 means accepted.
  if (!spi_txrx(0xFFU, &rx)) {
    spi2_sd_cs_high();
    return false;
  }
  if ((rx & 0x1FU) != DATA_RESP_ACCEPTED) {
    spi2_sd_cs_high();
    return false;
  }

  if (!sd_wait_not_busy(200000U)) {
    spi2_sd_cs_high();
    return false;
  }

  spi2_sd_cs_high();
  // Extra clocks after CS high keeps bus state clean for next command.
  if (!spi_txrx(0xFFU, &rx)) {
    return false;
  }

  return true;
}

bool sdcard_spi_read_block(uint32_t lba, uint8_t *data_512) {
  if (data_512 == 0) {
    return false;
  }

  uint8_t r1 = 0xFFU;
  uint8_t rx = 0U;

  uint32_t address = lba;
  if (!sd_high_capacity) {
    address = lba * SDCARD_BLOCK_SIZE;
  }

  spi2_sd_cs_low();

  if (!sd_send_cmd(CMD17, address, 0x01U, &r1)) {
    spi2_sd_cs_high();
    return false;
  }
  if (r1 != 0x00U) {
    spi2_sd_cs_high();
    return false;
  }

  if (!sd_read_data_block(TOKEN_SINGLE_BLOCK_READ, data_512, SDCARD_BLOCK_SIZE, 100000U)) {
    spi2_sd_cs_high();
    return false;
  }

  spi2_sd_cs_high();
  if (!spi_txrx(0xFFU, &rx)) {
    return false;
  }

  return true;
}

bool sdcard_spi_get_sector_count(uint32_t *sector_count) {
  if (sector_count == 0) {
    return false;
  }

  if (sd_sector_count == 0U) {
    return false;
  }

  *sector_count = sd_sector_count;
  return true;
}
