#include "sdcard_spi.h"

#include "spi2_sd.h"

#define SDCARD_BLOCK_SIZE 512U

// Standard SD commands in SPI mode.
#define CMD0   0U   // GO_IDLE_STATE: reset card to idle in SPI mode
#define CMD8   8U   // SEND_IF_COND: check voltage range and SD v2 support
#define CMD16 16U   // SET_BLOCKLEN: set block length (used for SDSC cards)
#define CMD17 17U   // READ_SINGLE_BLOCK: read one 512-byte data block
#define CMD24 24U   // WRITE_BLOCK: write one 512-byte data block
#define CMD55 55U   // APP_CMD prefix: next command is application-specific
#define CMD58 58U   // READ_OCR: read OCR/capacity status bits

#define ACMD41 41U  // SD_SEND_OP_COND: complete card initialization

// Data token for single block write in SPI mode.
#define TOKEN_SINGLE_BLOCK_WRITE 0xFEU
#define TOKEN_SINGLE_BLOCK_READ  0xFEU

// Data response token low bits after write.
#define DATA_RESP_ACCEPTED 0x05U

static bool sd_high_capacity = false;

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

bool sdcard_spi_init(void) {
  uint8_t r1 = 0xFFU;
  uint8_t rx = 0U;

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

  // Repeatedly issue ACMD41 until card leaves idle state.
  for (uint32_t tries = 0; tries < 2000U; tries++) {
    if (!sd_send_acmd(ACMD41, 0x40000000U, &r1)) {
      spi2_sd_cs_high();
      return false;
    }
    if (r1 == 0x00U) {
      break;
    }
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

  // Wait for start token 0xFE.
  for (uint32_t i = 0; i < 100000U; i++) {
    if (!spi_txrx(0xFFU, &rx)) {
      spi2_sd_cs_high();
      return false;
    }
    if (rx == TOKEN_SINGLE_BLOCK_READ) {
      break;
    }
  }
  if (rx != TOKEN_SINGLE_BLOCK_READ) {
    spi2_sd_cs_high();
    return false;
  }

  for (uint32_t i = 0; i < SDCARD_BLOCK_SIZE; i++) {
    if (!spi_txrx(0xFFU, &data_512[i])) {
      spi2_sd_cs_high();
      return false;
    }
  }

  // Ignore CRC bytes in SPI mode when CRC is disabled.
  if (!spi_txrx(0xFFU, &rx) || !spi_txrx(0xFFU, &rx)) {
    spi2_sd_cs_high();
    return false;
  }

  spi2_sd_cs_high();
  if (!spi_txrx(0xFFU, &rx)) {
    return false;
  }

  return true;
}
