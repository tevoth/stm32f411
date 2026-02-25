#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "gpio.h"
#include "system_init.h"
#include "tim2_1hz_init.h"

#define TIM_WAIT_LIMIT 100000U

// Wait until status bits are set; false indicates timeout.
static bool tim_wait_set(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = TIM_WAIT_LIMIT;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

int main(void) {
  system_init();
  tim2_1hz_init();
  led_init();
  while(1) {
    // TODO: UIF polling can miss an update if another overflow occurs
    // between observing UIF and clearing it; migrate to CNT rollover or IRQ.
    if (!tim_wait_set(&TIM2->SR, TIM_SR_UIF)) {
      continue;
    }
    TIM2->SR = ~TIM_SR_UIF; // reset UIF flag without RMW on SR
    led_toggle();
  }
  return 0;
}
