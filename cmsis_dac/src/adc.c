#include <stdint.h>
#include "stm32f4xx.h"

void adc_init(void) {

  // enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // set pin PA1 to analog function
  GPIOA->MODER |= GPIO_MODER_MODER1_0;
  GPIOA->MODER |= GPIO_MODER_MODER1_1;
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD1_Msk;
  
  // enable clock access to ADC unit
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  RCC->APB2RSTR |= RCC_APB2RSTR_ADCRST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_ADCRST;

  // Select ADC channel PA1
  ADC1->SQR3 &= ~(ADC_SQR3_SQ1_Msk); 
  ADC1->SQR3 |= ADC_SQR3_SQ1_0;

  // Set conversion sequence length
  ADC1->SQR1 &= ~(ADC_SQR1_L_Msk);

  // Set sample time (84 cycles)
  ADC1->SMPR2 &= ~(7 << (1 * 3));
  ADC1->SMPR2 |= (4 << (1 * 3));

  // Enable ADC from a known CR2 baseline.
  ADC1->CR2 = ADC_CR2_ADON;

}

void adc_start(void) {
  // Continuous conversion + software start from deterministic config.
  ADC1->CR2 = ADC_CR2_ADON | ADC_CR2_CONT | ADC_CR2_SWSTART;
}

uint32_t adc_read(void) {
  while(!(ADC1->SR & ADC_SR_EOC)){}
  uint32_t value = ADC1->DR;
  return value;
}
