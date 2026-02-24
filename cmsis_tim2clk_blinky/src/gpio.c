#include "gpio.h"

#define BTN_PIN       (1U<<0)
#define LED_PIN       (1U<<13)

static bool led_is_on = false;

void button_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  // set button pull-down
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0_0);
  GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR0_1);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_0);
  GPIOA->MODER &= ~(GPIO_MODER_MODER0_1);
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0);
} 

bool get_button_state() {
  // Pull-down on PA0 means pressed = high level.
  return (GPIOA->IDR & BTN_PIN) != 0U;
}

void led_init() {
  RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOCEN;
  GPIOC->PUPDR  |=  (GPIO_PUPDR_PUPDR13_0);
  GPIOC->PUPDR  &= ~(GPIO_PUPDR_PUPDR13_1);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);
  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_1);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT13);
  GPIOC->BSRR = GPIO_BSRR_BR13;
  led_is_on = false;
}


void led_on() {
  // set
  GPIOC->BSRR = GPIO_BSRR_BS13;
  led_is_on = true;
}

void led_off() {
  // reset
  GPIOC->BSRR = GPIO_BSRR_BR13;
  led_is_on = false;
}

void led_toggle() {
  led_is_on = !led_is_on;
  GPIOC->BSRR = led_is_on ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13;
}
