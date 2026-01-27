#include <stdint.h>
#include "stm32f4xx.h"

/*
typedef struct {
  volatile uint32_t MODER;      // 0x00
  volatile uint32_t OTYPER;     // 0x04
  volatile uint32_t OSPEEDR;    // 0x08
  volatile uint32_t PUPDR;      // 0x0C
  volatile uint32_t IDR;        // 0x10
  volatile uint32_t ODR;        // 0x14
  volatile uint32_t BSRR;       // 0x18
  volatile uint32_t LCKR;       // 0x1C
  volatile uint32_t AFRL;       // 0x20
  volatile uint32_t AFRH;       // 0x24
} gpio_t;

typedef struct {
  volatile uint32_t CR;         // 0x00
  volatile uint32_t PLLCFGR;    // 0x04
  volatile uint32_t CFGR;       // 0x08
  volatile uint32_t CIR;        // 0x0C
  volatile uint32_t AHB1RSTR;   // 0x10
  volatile uint32_t AHB2RSTR;   // 0x14
  volatile uint32_t RESERVE0;   // 0x18
  volatile uint32_t RESERVE1;   // 0x1C
  volatile uint32_t APB1RSTR;   // 0x20
  volatile uint32_t APB2RSTR;   // 0x24
  volatile uint32_t RESERVE2;   // 0x28
  volatile uint32_t RESERVE3;   // 0x2C
  volatile uint32_t AHB1ENR;    // 0x30
  volatile uint32_t AHB2ENR;    // 0x34
  volatile uint32_t RESERVE4;   // 0x38
  volatile uint32_t RESERVE5;   // 0x3C
  volatile uint32_t APB1ENR;    // 0x40
  volatile uint32_t APB2ENR;    // 0x44
  volatile uint32_t RESERVE6;   // 0x48
  volatile uint32_t RESERVE7;   // 0x4C
  volatile uint32_t AHB1LPENR;  // 0x50
  volatile uint32_t AHB2LPENR;  // 0x54
  volatile uint32_t RESERVE8;   // 0x58
  volatile uint32_t RESERVE9;   // 0x5C
  volatile uint32_t APB1LPENR;  // 0x60
  volatile uint32_t APB2LPENR;  // 0x64
  volatile uint32_t RESERVE10;  // 0x68
  volatile uint32_t RESERVE11;  // 0x6C
  volatile uint32_t BDCR;       // 0x70
  volatile uint32_t CSR;        // 0x74
  volatile uint32_t RESERVE12;  // 0x78
  volatile uint32_t RESERVE13;  // 0x7C
  volatile uint32_t SSCGR;      // 0x80
  volatile uint32_t PLLI2SCFGR; // 0x84
  volatile uint32_t DCKCFGR;    // 0x8C
} timer_t;

#define GPIOC_BASE 0x40020800UL
#define GPIOC ((gpio_t *)GPIOC_BASE)

#define TIMER_BASE 0x40023800UL
#define TIMER ((timer_t *)TIMER_BASE)
*/
  

#define GPIOCEN       (1U<<2)
#define PIN13         (1U<<13)
#define LED_PIN       PIN13

int main(void) {
  while(1) {
    //  Enable clock access to GPIOA
    RCC->AHB1ENR |= GPIOCEN;

    GPIOC->MODER  |= (1U<<26);  //  19: Set bit 10 to 1
    GPIOC->MODER  &= ~(1U<<27); //  20: Set bit 11 to 0

    while(1) {
      // toggle LED 
      GPIOC->ODR ^= LED_PIN;
      for(int i = 0; i < 5000000; i++){}
    }
  }
  return 1;
}
