#pragma once

#include <stdbool.h>
#include <stdint.h>

bool max6675_read_raw(uint16_t *raw);
bool max6675_thermocouple_open(uint16_t raw);
int32_t max6675_temp_c_x100(uint16_t raw);
