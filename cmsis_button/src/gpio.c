#include "gpio.h"

#define LED_BS13      (1U<<13)
#define LED_BR13      (1U<<29)
#define BTN_PIN       (1U<<0)

void button_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  // set button pull-up
  GPIOA->PUPDR |=  (1U<<0);
  GPIOA->PUPDR &= ~(1U<<1);
  GPIOA->OTYPER &= ~(1U<<0);
  GPIOA->MODER &= ~(GPIO_MODER_MODER1_0);
  GPIOA->MODER &= ~(GPIO_MODER_MODER1_1);
  //GPIOA->MODER &= ~(GPIO_MODER_MODER1_Msk);
} 

bool get_button_state() {
  if(GPIOA->IDR & BTN_PIN) {
    return true;
  } else {
    return false;
  }
}

void led_init() {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  GPIOA->OTYPER &= ~(1U<<13);
  GPIOA->PUPDR |=  (1U<<26);
  GPIOA->PUPDR &= ~(1U<<27);
  GPIOC->MODER |=  (GPIO_MODER_MODER13_0);
  GPIOC->MODER &= ~(GPIO_MODER_MODER13_1);
}

void led_on() {
  GPIOC->BSRR |= LED_BS13;
}

void led_off() {
  GPIOC->BSRR |= LED_BR13;
}
