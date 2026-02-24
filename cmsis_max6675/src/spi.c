#include "spi.h"

void spi_gpio_init(void) {
  // enable clock access to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // Set PA5, PA6, PA7 to alternate-function mode
  for (uint16_t pin = 5; pin < 8; pin++) {
    GPIOA->MODER &= ~(1U << (pin * 2));
    GPIOA->MODER |=  (2U << (pin * 2));
  }
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT5 | GPIO_OTYPER_OT7);
  GPIOA->PUPDR  &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD6_Msk | GPIO_PUPDR_PUPD7_Msk);

  // Set PA4 as output pin (chip-select)
  GPIOA->MODER |=  (1U << (4 * 2));
  GPIOA->MODER &= ~(2U << (4 * 2));
  GPIOA->OTYPER &= ~(GPIO_OTYPER_OT4);
  GPIOA->PUPDR  &= ~(GPIO_PUPDR_PUPD4_Msk);

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
  // MAX6675 uses SPI mode 0 (CPOL=0, CPHA=0).
  cr1 |= SPI_CR1_DFF; // 16-bit frame format
  cr1 |= SPI_CR1_SSI | SPI_CR1_SSM; // software slave management
  SPI1->CR1 = cr1;

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
  GPIOA->BSRR = GPIO_BSRR_BR4;
}

void cs_disable(void) {
  GPIOA->BSRR = GPIO_BSRR_BS4;
}
