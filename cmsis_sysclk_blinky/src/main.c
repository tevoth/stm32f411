#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "gpio.h"
#include "system_init.h"

int main(void) {
  system_init();
  led_init();
  while(1) {
    // toggle LED 
    led_toggle();
    delay(20000);
  }
  return 0;
}
