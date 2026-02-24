#include "gpio.h"

#define BTN_PIN       (1U<<0)
#define LED_PIN       (1U<<13)

void button_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  // set button pull-up
  GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR0_0);
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0_1);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_0);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_1);
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0);
} 

bool get_button_state() {
  // Pull-up on PA0 means pressed = low level.
  return (GPIOA->IDR & BTN_PIN) == 0U;
}

void led_init() {
  RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOCEN;
  GPIOC->PUPDR  |=  (GPIO_PUPDR_PUPDR13_0);
  GPIOC->PUPDR  &= ~(GPIO_PUPDR_PUPDR13_1);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_1);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT13);
}

void led_on() {
  // set
  GPIOC->BSRR = GPIO_BSRR_BS13;
}

void led_off() {
  // reset
  GPIOC->BSRR = GPIO_BSRR_BR13;
}

void led_toggle() {
  // toggle
  GPIOC->ODR ^= LED_PIN; 
}
