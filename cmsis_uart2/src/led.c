#include "led.h"
#include <stdbool.h>

static bool led_is_on = false;

void led_init(void) {
  // Enable clock access to GPIOC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_Msk);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT13);
  GPIOC->BSRR = GPIO_BSRR_BR13;
  led_is_on = false;
}


void led_toggle(void) {
  led_is_on = !led_is_on;
  GPIOC->BSRR = led_is_on ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13;
}
