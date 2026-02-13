#include "spi.h"
#define SPI1EN (1U<<12)
#define GPIOAEN (1U<<0)
#define SR_TXE  (1U<<1)
#define SR_RXNE (1U<<1)
#define SR_BSY  (1U<<8)

void spi_gpio_init(void) {
  // enable clock access to GPIOA
  RCC->AHB1ENR |= GPIOAEN;

  // Set PA5 AF
  for (uint16_t pin = 5; pin < 8; pin++) {
    GPIOA->MODER &= ~(1U << (pin * 2));
    GPIOA->MODER |=  (2U << (pin * 2));
  }

  // Set PA9 as output pin
  GPIOA->MODER |=  (1U << (9 * 2));
  GPIOA->MODER &= ~(2U << (9 * 2));

  // Set PA5, PA6, PA7 to AF type SPI
  for (uint16_t pin = 5; pin < 8; pin++) {
    GPIOA->AFR[0] |=  (1U << (pin * 4));
    GPIOA->AFR[0] &= ~(2U << (pin * 4));
    GPIOA->AFR[0] |=  (4U << (pin * 4));
    GPIOA->AFR[0] &= ~(8U << (pin * 4));
  }
}

void spi1_config(void) {
  // Enable clock access to SPI1
  RCC->APB2ENR |= SPI1EN;
  
  // Set clock to fPClk/4
  SPI1->CR1 |=  (1U << 3);
  SPI1->CR1 &= ~(1U << 4);
  SPI1->CR1 &= ~(1U << 5);

  // Set CPOL and CPHA to 1
  SPI1->CR1 |= (1U << 0);
  SPI1->CR1 |= (1U << 1);

  // Set MSB first
  SPI1->CR1 &= ~(1U << 7);
  
  // Set Master
  SPI1->CR1 |= (1U << 2);

  // Set 8-bit data mode
  SPI1->CR1 &= ~(1U << 11);

  // Select software slave management 
  SPI1->CR1 |= (1 << 8);
  SPI1->CR1 |= (1 << 9);

  // Enable SPI module
  SPI1->CR1 |= (1 << 6);
}

void spi1_transmit(uint8_t *data, uint32_t size) {
  uint32_t i = 0;
  uint8_t  temp;
  while (i < size) {
    // wait for TXE set
    while (!(SPI1->SR & (SR_TXE))) {}
    
    // data ready, right to data register
    SPI1->DR = data[i];
    i++;
  }
  // wait until TXE is set
  while (!(SPI1->SR & (SR_TXE))) {}

  // wait for BUSY flag to reset
  while ((SPI1->SR & (SR_BSY))) {}

  // clear flags
  temp = SPI1->DR;
  temp = SPI1->SR;
}

void spi1_receive(uint8_t *data, uint32_t size) {
  while (size) {
    // set dummy data
    SPI1->DR = 0;
    // wait for RXNE flag to be set
    while (!(SPI1->SR & (SR_RXNE))) {}
    // read data
    *data++ = (SPI1->DR);
    size--;
  }
}

void cs_enable(void) {
  GPIOA->ODR &= ~(1U << 9);
}

void cs_disable(void) {
  GPIOA->ODR |=  (1U << 9);
}
