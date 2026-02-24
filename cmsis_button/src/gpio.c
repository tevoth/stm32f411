#include "gpio.h"

#define BTN_PIN       (1U<<0)

void button_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  // configure PA0 as input with pull-down so pressed reads high
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0_0);
  GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR0_1);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_0);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_1);
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0);
} 

bool get_button_state() {
  return (GPIOA->IDR & BTN_PIN) != 0U;
}

void led_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  GPIOC->PUPDR  |=  (GPIO_PUPDR_PUPDR13_0);
  GPIOC->PUPDR  &= ~(GPIO_PUPDR_PUPDR13_1);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_1);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT13);
  GPIOC->BSRR = GPIO_BSRR_BR13;
}

void led_on() {
  // set
  GPIOC->BSRR = GPIO_BSRR_BS13;
}

void led_off() {
  // reset
  GPIOC->BSRR = GPIO_BSRR_BR13;
}
