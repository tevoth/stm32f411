#include "led.h"

void led_init(void) {
  //  Enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_Msk);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
}

void led_toggle(void) {
  GPIOC->ODR ^= LED_PIN;
}
