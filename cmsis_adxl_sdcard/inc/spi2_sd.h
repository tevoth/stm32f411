#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx.h"

// Initialize GPIO + SPI2 for microSD card wiring.
void spi2_sd_init(void);

// Run SPI2 at very low speed for SD card power-up/initialization.
void spi2_sd_set_slow_clock(void);

// Run SPI2 at a faster speed after card initialization.
void spi2_sd_set_fast_clock(void);

// Full-duplex transfer of one byte over SPI2.
// Returns true on success and stores the received byte in rx.
bool spi2_sd_transfer(uint8_t tx, uint8_t *rx);

// Assert/deassert microSD chip-select (active low).
void spi2_sd_cs_low(void);
void spi2_sd_cs_high(void);
