#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
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
    uint32_t value = adc_read();
    if (value == UINT32_MAX) {
      printf("ADC read timeout\n");
      systick_msec_delay(100);
      continue;
    }
    printf("HELLO FROM STM32...%" PRIu32 "\n", value);
    systick_msec_delay(100);
  }
  return 0;
}
