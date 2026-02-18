#pragma once

#include <stdint.h>
#include "stm32f4xx.h"

void spi_gpio_init(void);
void spi1_config(void);
uint16_t spi1_transfer16(uint16_t tx_data);
void cs_enable(void);
void cs_disable(void);
