#include "button.h"

#include "gpio.h"

static void delay_1ms(void) {
  SysTick->CTRL = 0U;
  SysTick->VAL = 0U;
  SysTick->LOAD = 16000U - 1U; // 1 ms at 16 MHz HSI
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
  while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {}
  SysTick->CTRL = 0U;
}

bool button_get_state_debounced(void) {
  bool candidate = get_button_state();
  uint32_t stable_samples = 0U;

  while (stable_samples < 10U) {
    delay_1ms();
    bool now = get_button_state();
    if (now == candidate) {
      stable_samples++;
    } else {
      candidate = now;
      stable_samples = 0U;
    }
  }

  return candidate;
}
