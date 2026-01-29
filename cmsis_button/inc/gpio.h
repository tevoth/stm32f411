#pragma once

#include <stdbool.h>
#include "stm32f4xx.h"

void button_init();
bool get_button_state();
void led_init();
void led_on();
void led_off();
