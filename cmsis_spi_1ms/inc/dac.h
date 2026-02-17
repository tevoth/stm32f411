#pragma once

#include <stdint.h>

void dac_init(void);
void dac_start(void);
uint32_t dac_read(void);
