#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio.h"
#include "system_init.h"
#include "tim2_1hz_init.h"

int main(void) {
  system_init();
  tim2_1hz_init();
  led_init();
  while(1) {
    // TODO: UIF polling can miss an update if another overflow occurs
    // between observing UIF and clearing it; migrate to CNT rollover or IRQ.
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
      TIM2->SR = ~TIM_SR_UIF; // reset UIF flag without RMW on SR
      led_toggle();
    }
  }
  return 0;
}
