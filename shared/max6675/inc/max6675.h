#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  MAX6675_STATUS_OK = 0,
  MAX6675_STATUS_THERMOCOUPLE_OPEN,
  MAX6675_STATUS_BUS_INVALID,
  MAX6675_STATUS_TIMEOUT
} max6675_status_t;

bool max6675_read_raw(uint16_t *raw);
max6675_status_t max6675_read_status(uint16_t *raw);
bool max6675_thermocouple_open(uint16_t raw);
int32_t max6675_temp_c_x100(uint16_t raw);
