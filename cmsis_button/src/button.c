#include "button.h"

#include "gpio.h"

#define BUTTON_DEBOUNCE_STABLE_SAMPLES 10U
#define BUTTON_DEBOUNCE_TIMEOUT_MS 50U

// Preserves the last confirmed debounced state across timeout exits.
static bool last_stable_state = false;

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
  uint32_t elapsed_ms = 0U;

  while ((stable_samples < BUTTON_DEBOUNCE_STABLE_SAMPLES) &&
         (elapsed_ms < BUTTON_DEBOUNCE_TIMEOUT_MS)) {
    delay_1ms();
    elapsed_ms++;
    bool now = get_button_state();
    if (now == candidate) {
      stable_samples++;
    } else {
      candidate = now;
      stable_samples = 0U;
    }
  }

  if (stable_samples >= BUTTON_DEBOUNCE_STABLE_SAMPLES) {
    last_stable_state = candidate;
  }

  return last_stable_state;
}
