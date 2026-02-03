#include <stdint.h>
#include "stm32f4xx.h"

#define CTRL_COUNTFLAG (1U<<16)

void delay(uint32_t delay) {
  SysTick->CTRL = 0;
  SysTick->VAL  = 0;
  SysTick->LOAD = 15999;
  SysTick->CTRL = 5;

  for (uint32_t i = 0; i < delay; i++) {
    while((SysTick->CTRL & CTRL_COUNTFLAG) == 0) {}
  }
  SysTick->CTRL = 0;
}
   
