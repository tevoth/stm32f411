#include <stdint.h>
#include "stm32f4xx.h"

void dac_init(void) {

  // enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // set pin PA1 to analog function
  GPIOA->MODER |= GPIO_MODER_MODER1_0;
  GPIOA->MODER |= GPIO_MODER_MODER1_1;
  
  // enable clock access to ADC unit
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

  // Set conversion sequence start at first ADC channel (PA1)???
  ADC1->SQR3 &= ~(ADC_SQR3_SQ1_Msk);

  // Set conversion sequence length
  ADC1->SQR1 &= ~(ADC_SQR1_L_Msk);

  // Enable ADC 
  ADC1->CR2 |= ADC_CR2_ADON; 

}

void dac_start(void) {
  // enable continuous conversion
  ADC1->CR2 |= ADC_CR2_CONT;
  // start aquisition
  ADC1->CR2 |= ADC_CR2_SWSTART;
}

uint32_t dac_read(void) {
  while(!(ADC1->SR & ADC_SR_EOC)){}
  uint32_t value = ADC1->DR;
  return value;
}
