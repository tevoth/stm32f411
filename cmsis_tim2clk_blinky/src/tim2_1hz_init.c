#include <stdint.h>
#include "stm32f4xx.h"

#define PRESCL 1600
#define PERIOD 10000

void tim2_1hz_init(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable timer 2

  // Force TIM2 back to reset state so warm/debug resets start from known config.
  RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

  TIM2->PSC = PRESCL - 1;
  TIM2->CNT = 0;
  TIM2->EGR = TIM_EGR_UG; // update pre-scaler NOW
  TIM2->SR = ~TIM_SR_UIF; // clear UIF raised by UG without RMW on SR
  TIM2->ARR = PERIOD - 1;
  TIM2->CR1 = TIM_CR1_CEN; // start timer
  // timer debugging DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP;
}
   
