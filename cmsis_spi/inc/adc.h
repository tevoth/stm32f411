#pragma once

#include <stdint.h>

void adc_init(void);
void adc_start(void);
uint32_t adc_read(void);
