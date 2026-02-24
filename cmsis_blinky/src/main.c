#include <stdint.h>
#include "stm32f4xx.h"
#include "system_init.h"

#define GPIOCEN       (1U<<2)
#define PIN13         (1U<<13)
#define LED_PIN       PIN13

int main(void) {
  system_init();
  // Enable clock access to GPIOC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  GPIOC->MODER  &= ~(GPIO_MODER_MODER13_Msk);
  GPIOC->MODER  |=  (GPIO_MODER_MODER13_0);

  while(1) {
    if (GPIOC->ODR & LED_PIN) {
      GPIOC->BSRR = GPIO_BSRR_BR13;
    } else {
      GPIOC->BSRR = GPIO_BSRR_BS13;
    }
    for(volatile int i = 0; i < 5000000; i++){}
  }
  return 1;
}
