#include "stm32f4xx.h"

#define LED_PIN (1U << 13)

static void system_init(void) {
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

  // Normalize bus prescalers to /1 for deterministic timing.
  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_HSI;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}
}
  
int main(void) {
  system_init();
  // Enable clock access to GPIOC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

  GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
  GPIOC->MODER |= GPIO_MODER_MODER13_0;

  while(1) {
    // toggle LED 
    GPIOC->ODR ^= LED_PIN;
    for(volatile int i = 0; i < 5000000; i++){}
  }
  return 1;
}
