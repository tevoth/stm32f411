#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio.h"
#include "system_init.h"
#include "systick_msec_delay.h"

int main(void) {

  system_init();
  led_init();
  while(1) {
    // toggle LED 
    led_toggle();
    systick_msec_delay(5000);
  }
  return 0;
}
