#include "spi.h"

void spi_gpio_init(void) {
  // enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // Set PA5, PA6, PA7 to alternate-function mode
  for (uint16_t pin = 5; pin < 8; pin++) {
    GPIOA->MODER &= ~(1U << (pin * 2));
    GPIOA->MODER |=  (2U << (pin * 2));
  }

  // Set PA4 as output pin (chip-select)
  GPIOA->MODER |=  (1U << (4 * 2));
  GPIOA->MODER &= ~(2U << (4 * 2));

  // default CS high (inactive)
  GPIOA->BSRR = GPIO_BSRR_BS4;

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

  // Reset SPI1 to remove stale peripheral state on warm/debug reset paths.
  RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;

  // Compose CR1 from a known baseline.
  uint32_t cr1 = 0U;
  cr1 |= SPI_CR1_MSTR; // master mode
  cr1 |= SPI_CR1_BR_0; // fPCLK/4
  cr1 |= SPI_CR1_CPHA | SPI_CR1_CPOL; // SPI mode 3
  cr1 |= SPI_CR1_SSI | SPI_CR1_SSM; // software slave management
  // DFF remains 0 for 8-bit mode.
  SPI1->CR1 = cr1;

  // Enable SPI module
  SPI1->CR1 |= (SPI_CR1_SPE);
}

void spi1_transmit(uint8_t *data, uint32_t size) {
  uint32_t i = 0;
  while (i < size) {
    // wait for TXE set
    while (!(SPI1->SR & (SPI_SR_TXE))) {}
    
    // data ready, right to data register
    *(__IO uint8_t *)&SPI1->DR = data[i];
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
    *(__IO uint8_t *)&SPI1->DR = 0U;
    // wait for RXNE flag to be set
    while (!(SPI1->SR & (SPI_SR_RXNE))) {}
    // read data
    *data++ = *(__IO uint8_t *)&SPI1->DR;
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
  GPIOA->BSRR = GPIO_BSRR_BR4;
}

void cs_disable(void) {
  GPIOA->BSRR = GPIO_BSRR_BS4;
}
