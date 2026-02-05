#pragma once

#include "stm32f4xx.h"

#define PIN13         (1U<<13)
#define LED_PIN       PIN13

void led_init(void);
void led_toggle(void);
