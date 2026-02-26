#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx.h"

void spi_gpio_init(void);
void spi1_config(void);
bool spi1_transmit(uint8_t *data, uint32_t size);
bool spi1_receive(uint8_t *data, uint32_t size);
void cs_enable(void);
void cs_disable(void);
