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

  // Set 16-bit data mode
  SPI1->CR1 |= SPI_CR1_DFF;

  // Select software slave management 
  SPI1->CR1 |= (SPI_CR1_SSI);
  SPI1->CR1 |= (SPI_CR1_SSM);

  // Enable SPI module
  SPI1->CR1 |= (SPI_CR1_SPE);
}

uint16_t spi1_transfer16(uint16_t tx_data) {
  // Wait until transmit buffer is empty.
  while (!(SPI1->SR & (SPI_SR_TXE))) {}

  // 16-bit access is required when DFF=1.
  *(__IO uint16_t *)&SPI1->DR = tx_data;

  // Wait for the received 16-bit frame.
  while (!(SPI1->SR & (SPI_SR_RXNE))) {}
  uint16_t rx_data = *(__IO uint16_t *)&SPI1->DR;

  // Wait until transfer is fully complete on the wire.
  while (!(SPI1->SR & (SPI_SR_TXE))) {}
  while (SPI1->SR & (SPI_SR_BSY)) {}

  // Final SR read keeps status handling consistent with other paths.
  (void)SPI1->SR;

  return rx_data;
}

void cs_enable(void) {
  GPIOA->ODR &= ~(1U << 4);
}

void cs_disable(void) {
  GPIOA->ODR |=  (1U << 4);
}
