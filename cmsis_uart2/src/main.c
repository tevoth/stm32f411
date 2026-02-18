#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "system_init.h"

int main(void) {
  system_init();
  led_init();
  uart_init();
  while(1) {
    led_toggle();
    printf("HELLO FROM STM32...\n"); 
    for(volatile int i = 0; i < 500000; i++) {}
  }
  return 0;
}
