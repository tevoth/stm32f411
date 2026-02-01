#include "gpio.h"

#define GPIOCEN       (1U<<2)
#define GPIOAEN       (1U<<0)
#define LED_BS13      (1U<<13)
#define LED_BR13      (1U<<29)
#define BTN_PIN       (1U<<0)

void button_init() {
  RCC->AHB1ENR |= GPIOAEN;
  // set button pull-up
  GPIOA->PUPDR |=  (1U<<0);
  GPIOA->PUPDR &= ~(1U<<1);
  GPIOA->OTYPER &= ~(1U<<0);
  GPIOA->MODER &= ~(1U<<0);
  GPIOA->MODER &= ~(1U<<1);
} 

bool get_button_state() {
  if(GPIOA->IDR & BTN_PIN) {
    return true;
  } else {
    return false;
  }
}

void led_init() {
  RCC->AHB1ENR |= GPIOCEN;
  GPIOA->OTYPER &= ~(1U<<13);
  // set led pull-up
  GPIOA->PUPDR |=  (1U<<26);
  GPIOA->PUPDR &= ~(1U<<27);
  GPIOC->MODER |=  (1U<<26);  //  19: Set bit 10 to 1
  GPIOC->MODER &= ~(1U<<27); //  20: Set bit 11 to 0
}

void led_on() {
  GPIOC->BSRR |= LED_BS13;
}

void led_off() {
  GPIOC->BSRR |= LED_BR13;
}
