#include "max6675.h"
#include "spi.h"

static bool max6675_frame_valid(uint16_t raw)
{
  // MAX6675 reserved bits must read 0.
  return (raw & ((1U << 15) | (1U << 1))) == 0U;
}

bool max6675_read_raw(uint16_t *raw)
{
  if (raw == 0) {
    return false;
  }

  // Retry once to filter occasional invalid bus frames.
  for (uint32_t attempt = 0; attempt < 2U; attempt++) {
    cs_enable();
    uint16_t frame = spi1_transfer16(0x0000U);
    cs_disable();

    if (max6675_frame_valid(frame)) {
      *raw = frame;
      return true;
    }
  }

  return false;
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
