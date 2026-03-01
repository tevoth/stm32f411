#pragma once

#include <stdbool.h>
#include <stdint.h>

// Initialize the SD card in SPI mode.
// Returns true when the card is ready for block writes.
bool sdcard_spi_init(void);

// Write one 512-byte logical block.
// LBA means Logical Block Address (block index, not byte address).
bool sdcard_spi_write_block(uint32_t lba, const uint8_t *data_512);

// Read one 512-byte logical block.
// LBA means Logical Block Address (block index, not byte address).
bool sdcard_spi_read_block(uint32_t lba, uint8_t *data_512);

// Query total number of 512-byte logical blocks on the initialized card.
bool sdcard_spi_get_sector_count(uint32_t *sector_count);
