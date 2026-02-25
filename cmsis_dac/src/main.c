#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "adc.h"
#include "systick_msec_delay.h"
#include "system_init.h"

int main(void) {
  system_init();
  led_init();
  uart_init();
  adc_init();
  adc_start();
  while(1) {
    led_toggle();
    volatile unsigned int value = adc_read(); 
    printf("HELLO FROM STM32...%u\n",value); 
    systick_msec_delay(100);
  }
  return 0;
}
