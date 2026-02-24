#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "system_init.h"

#define GPIOCEN       (1U<<2)
#define PIN13         (1U<<13)
#define LED_PIN       PIN13

static bool led_is_on = false;

int main(void) {
  system_init();
  // Enable clock access to GPIOC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_Msk);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
  GPIOC->BSRR = GPIO_BSRR_BR13;
  led_is_on = false;

  while(1) {
    led_is_on = !led_is_on;
    GPIOC->BSRR = led_is_on ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13;
    for(volatile int i = 0; i < 5000000; i++){}
  }
  return 1;
}
