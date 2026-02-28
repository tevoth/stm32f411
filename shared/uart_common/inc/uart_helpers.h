#pragma once

#include <stdbool.h>
#include <stdint.h>

// Wait until all bits in mask are set in *reg; false indicates timeout.
static inline bool uart_wait_set_limit(volatile uint32_t *reg, uint32_t mask, uint32_t limit) {
  uint32_t timeout = limit;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

// Compute BRR value using nearest-integer rounding.
static inline uint16_t uart_compute_bd(uint32_t periph_clk, uint32_t baudrate) {
  return (uint16_t)((periph_clk + (baudrate / 2U)) / baudrate);
}
