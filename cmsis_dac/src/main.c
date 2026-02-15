#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "dac.h"

int main(void) {
  led_init();
  uart_init();
  dac_init();
  dac_start();
  int count = 0;
  while(1) {
    led_toggle();
    volatile unsigned int value = dac_read(); 
    printf("HELLO FROM STM32...%u\n",value); 
    count = count + 1;
    for (volatile int i = 0; i < 1000000; i++) {}
  }
  return 0;
}
