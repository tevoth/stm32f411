#include "stm32f4xx.h"


void system_init(void)
{
    volatile int64_t rcc_cr = RCC->CR;
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    volatile int64_t rcc_cfgr = RCC->CFGR;
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

}

