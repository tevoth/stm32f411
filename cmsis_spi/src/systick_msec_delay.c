#include <stdint.h>
#include "stm32f4xx.h"

#define CTRL_COUNTFLAG (1U << 16)
#define SYSTICK_INPUT_HZ 16000000U

void systick_msec_delay(uint32_t delay_ms) {
  const uint32_t reload = (SYSTICK_INPUT_HZ / 1000U) - 1U;

  SysTick->CTRL = 0;
  SysTick->VAL = 0;
  SysTick->LOAD = reload;
  SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

  for (uint32_t i = 0; i < delay_ms; i++) {
    while ((SysTick->CTRL & CTRL_COUNTFLAG) == 0) {}
  }

  SysTick->CTRL = 0;
}
