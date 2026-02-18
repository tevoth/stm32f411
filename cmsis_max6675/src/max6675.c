#include "max6675.h"
#include "spi.h"

bool max6675_read_raw(uint16_t *raw)
{
  if (raw == 0) {
    return false;
  }

  uint8_t rx[2] = {0, 0};

  cs_enable();
  spi1_receive(rx, 2);
  cs_disable();

  *raw = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
  return true;
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
