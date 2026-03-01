#include "stm32f4xx.h"
#include <stdbool.h>

#define RCC_WAIT_LIMIT 100000U

bool system_init(void)
{
    RCC->CR |= RCC_CR_HSION;

    uint32_t timeout = RCC_WAIT_LIMIT;
    while (!(RCC->CR & RCC_CR_HSIRDY) && (timeout-- > 0U)) {}
    if ((RCC->CR & RCC_CR_HSIRDY) == 0U) {
        return false;
    }

    // Normalize bus prescalers so timing is deterministic after soft/debug resets.
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;

    timeout = RCC_WAIT_LIMIT;
    while (((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) && (timeout-- > 0U)) {}
    if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {
        return false;
    }

    return true;
}
