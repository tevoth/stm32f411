#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "system_init.h"
#include "systick_msec_delay.h"

int main(void) {
  if (!system_init()) {
    while (1) {}
  }
  led_init();
  uart_init();
  while(1) {
    led_toggle();
    printf("HELLO FROM STM32...\n"); 
    systick_msec_delay(100);
  }
  return 0;
}
