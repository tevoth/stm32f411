#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef bool (*spi_xfer_tx_fn_t)(uint8_t *data, uint32_t size);
typedef bool (*spi_xfer_rx_fn_t)(uint8_t *data, uint32_t size);
typedef void (*spi_xfer_cs_fn_t)(void);

// Run a chip-select scoped transmit operation.
static inline bool spi_cs_transmit(spi_xfer_cs_fn_t cs_enable_fn,
                                   spi_xfer_cs_fn_t cs_disable_fn,
                                   spi_xfer_tx_fn_t tx_fn,
                                   uint8_t *tx_data,
                                   uint32_t tx_size) {
  if ((cs_enable_fn == 0U) || (cs_disable_fn == 0U) || (tx_fn == 0U)) {
    return false;
  }

  cs_enable_fn();
  bool ok = tx_fn(tx_data, tx_size);
  cs_disable_fn();
  return ok;
}

// Run a chip-select scoped transmit then receive sequence.
static inline bool spi_cs_transmit_receive(spi_xfer_cs_fn_t cs_enable_fn,
                                           spi_xfer_cs_fn_t cs_disable_fn,
                                           spi_xfer_tx_fn_t tx_fn,
                                           spi_xfer_rx_fn_t rx_fn,
                                           uint8_t *tx_data,
                                           uint32_t tx_size,
                                           uint8_t *rx_data,
                                           uint32_t rx_size) {
  if ((cs_enable_fn == 0U) || (cs_disable_fn == 0U) || (tx_fn == 0U) || (rx_fn == 0U)) {
    return false;
  }

  cs_enable_fn();
  bool ok = tx_fn(tx_data, tx_size) && rx_fn(rx_data, rx_size);
  cs_disable_fn();
  return ok;
}
