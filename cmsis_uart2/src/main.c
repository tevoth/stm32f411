#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"

int main(void) {
  led_init();
  uart_init();
  while(1) {
    led_toggle();
    printf("HELLO FROM STM32...\n"); 
    for(int i = 0; i < 500000; i++) {}
  }
  return 0;
}
