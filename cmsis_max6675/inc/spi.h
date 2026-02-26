#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"

void spi_gpio_init(void);
void spi1_config(void);
uint16_t spi1_transfer16(uint16_t tx_data);
bool spi1_transfer16_checked(uint16_t tx_data, uint16_t *rx_data);
void cs_enable(void);
void cs_disable(void);
