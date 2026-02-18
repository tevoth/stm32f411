#include "spi.h"

void spi_gpio_init(void) {
  // enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // Set PA5 AF
  for (uint16_t pin = 5; pin < 8; pin++) {
    GPIOA->MODER &= ~(1U << (pin * 2));
    GPIOA->MODER |=  (2U << (pin * 2));
  }

  // Set PA4 as output pin (chip-select)
  GPIOA->MODER |=  (1U << (4 * 2));
  GPIOA->MODER &= ~(2U << (4 * 2));

  // default CS high (inactive)
  GPIOA->ODR |= (1U << 4);

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
  RCC->APB2ENR |= (RCC_APB2ENR_SPI1EN);
  
  // Set clock to fPClk/4
  SPI1->CR1 |=  (1U << 3);
  SPI1->CR1 &= ~(1U << 4);
  SPI1->CR1 &= ~(1U << 5);

  // MAX6675 uses SPI mode 0 (CPOL=0, CPHA=0)
  SPI1->CR1 &= ~(SPI_CR1_CPHA);
  SPI1->CR1 &= ~(SPI_CR1_CPOL);

  // Set MSB first
  SPI1->CR1 &= ~(SPI_CR1_LSBFIRST);
  
  // Set as master
  SPI1->CR1 |= (SPI_CR1_MSTR);

  // Set 8-bit data mode
  SPI1->CR1 &= ~(SPI_CR1_DFF);

  // Select software slave management 
  SPI1->CR1 |= (SPI_CR1_SSI);
  SPI1->CR1 |= (SPI_CR1_SSM);

  // Enable SPI module
  SPI1->CR1 |= (SPI_CR1_SPE);
}

void spi1_transmit(uint8_t *data, uint32_t size) {
  uint32_t i = 0;
  while (i < size) {
    // wait for TXE set
    while (!(SPI1->SR & (SPI_SR_TXE))) {}
    
    // data ready, right to data register
    SPI1->DR = data[i];
    i++;
  }
  // wait until TXE is set
  while (!(SPI1->SR & (SPI_SR_TXE))) {}

  // wait for BUSY flag to reset
  while ((SPI1->SR & (SPI_SR_BSY))) {}

  // clear flags
  (void)SPI1->DR;
  (void)SPI1->SR;
}

void spi1_receive(uint8_t *data, uint32_t size) {
  while (size) {
    // set dummy data set generate SPI clock
    while (!(SPI1->SR & (SPI_SR_TXE))) {}
    SPI1->DR = 0;
    // wait for RXNE flag to be set
    while (!(SPI1->SR & (SPI_SR_RXNE))) {}
    // read data
    *data++ = (SPI1->DR);
    size--;
  }

  // wait until the last frame has fully shifted out before deasserting CS
  while (!(SPI1->SR & (SPI_SR_TXE))) {}
  while ((SPI1->SR & (SPI_SR_BSY))) {}

  // clear flags
  (void)SPI1->DR;
  (void)SPI1->SR;
}

void cs_enable(void) {
  GPIOA->ODR &= ~(1U << 4);
}

void cs_disable(void) {
  GPIOA->ODR |=  (1U << 4);
}
