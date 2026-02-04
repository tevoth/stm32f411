#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio.h"
#include "system_init.h"
#include "tim2_1hz_init.h"

int main(void) {
//  system_init();
  tim2_1hz_init();
  led_init();
  while(1) {
    led_toggle();
    while(!(TIM2->SR & TIM_SR_UIF)){}
    TIM2->SR &= ~TIM_SR_UIF; // reset UIF flag
  }
  return 0;
}
