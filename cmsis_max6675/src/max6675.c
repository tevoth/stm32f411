#include "max6675.h"
#include "spi.h"
#include "spi_cs_xfer.h"

static bool max6675_frame_valid(uint16_t raw)
{
  // MAX6675 reserved bits must read 0.
  return (raw & ((1U << 15) | (1U << 1))) == 0U;
}

max6675_status_t max6675_read_status(uint16_t *raw)
{
  if (raw == 0) {
    return MAX6675_STATUS_TIMEOUT;
  }

  // Retry once to filter occasional invalid bus frames.
  for (uint32_t attempt = 0; attempt < 2U; attempt++) {
    uint8_t rx[2] = {0U, 0U};
    bool ok = spi_cs_transmit(cs_enable, cs_disable, spi1_receive, rx, 2U);

    if (!ok) {
      return MAX6675_STATUS_TIMEOUT;
    }

    uint16_t frame = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);

    if (!max6675_frame_valid(frame)) {
      continue;
    }

    *raw = frame;
    if (max6675_thermocouple_open(frame)) {
      return MAX6675_STATUS_THERMOCOUPLE_OPEN;
    }
    return MAX6675_STATUS_OK;
  }

  return MAX6675_STATUS_BUS_INVALID;
}

bool max6675_read_raw(uint16_t *raw)
{
  max6675_status_t status = max6675_read_status(raw);
  return (status == MAX6675_STATUS_OK) ||
         (status == MAX6675_STATUS_THERMOCOUPLE_OPEN);
}

bool max6675_thermocouple_open(uint16_t raw)
{
  return (raw & (1U << 2)) != 0U;
}

int32_t max6675_temp_c_x100(uint16_t raw)
{
  uint16_t temp_counts = (uint16_t)((raw >> 3) & 0x0FFFU);
  return (int32_t)temp_counts * 25;
}
