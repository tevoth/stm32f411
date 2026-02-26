#pragma once

#include <stdbool.h>
#include <stdint.h>

// Wait until all bits in mask are set in *reg; false indicates timeout.
static inline bool spi_wait_set_limit(volatile uint32_t *reg, uint32_t mask, uint32_t limit) {
  uint32_t timeout = limit;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

// Wait until all bits in mask are clear in *reg; false indicates timeout.
static inline bool spi_wait_clear_limit(volatile uint32_t *reg, uint32_t mask, uint32_t limit) {
  uint32_t timeout = limit;
  while (((*reg & mask) != 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) == 0U);
}
