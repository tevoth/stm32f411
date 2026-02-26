#include "spi.h"
#include <stdbool.h>

#define SPI_WAIT_LIMIT 100000U

// Wait until status bits are set; false indicates timeout.
static bool spi_wait_set(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = SPI_WAIT_LIMIT;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

// Wait until status bits are clear; false indicates timeout.
static bool spi_wait_clear(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = SPI_WAIT_LIMIT;
  while (((*reg & mask) != 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) == 0U);
}

static void spi_clear_status(void) {
  (void)SPI1->DR;
  (void)SPI1->SR;
}

// Best-effort timeout recovery so a failed transfer does not poison the next one.
static void spi_recover(void) {
  if (!spi_wait_clear(&SPI1->SR, SPI_SR_BSY)) {
    // Fallback if SPI remains busy/stuck: pulse SPE while preserving CR1 config.
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_SPE;
  }
  spi_clear_status();
}

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
  // Bias MISO high so unplugged sensor does not leave the input floating.
  GPIOA->PUPDR  |= GPIO_PUPDR_PUPD6_0;

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
  cr1 |= SPI_CR1_SSI | SPI_CR1_SSM; // software slave management
  SPI1->CR1 = cr1;

  // Enable SPI module
  SPI1->CR1 |= (SPI_CR1_SPE);
}

bool spi1_transmit(uint8_t *data, uint32_t size) {
  if ((data == 0U) && (size > 0U)) {
    return false;
  }

  uint32_t i = 0;
  while (i < size) {
    // Wait for TX buffer space with timeout guard.
    if (!spi_wait_set(&SPI1->SR, SPI_SR_TXE)) {
      spi_recover();
      return false;
    }
    
    // data ready, right to data register
    *(__IO uint8_t *)&SPI1->DR = data[i];
    i++;
  }
  // Wait until TXE is set for the last frame.
  if (!spi_wait_set(&SPI1->SR, SPI_SR_TXE)) {
    spi_recover();
    return false;
  }

  // Wait until SPI is no longer busy on the wire.
  if (!spi_wait_clear(&SPI1->SR, SPI_SR_BSY)) {
    spi_recover();
    return false;
  }

  spi_clear_status();

  return true;
}

bool spi1_receive(uint8_t *data, uint32_t size) {
  if ((data == 0U) && (size > 0U)) {
    return false;
  }

  while (size) {
    // set dummy data set generate SPI clock
    if (!spi_wait_set(&SPI1->SR, SPI_SR_TXE)) {
      spi_recover();
      return false;
    }
    *(__IO uint8_t *)&SPI1->DR = 0U;
    // wait for RXNE flag to be set
    if (!spi_wait_set(&SPI1->SR, SPI_SR_RXNE)) {
      spi_recover();
      return false;
    }
    // read data
    *data++ = *(__IO uint8_t *)&SPI1->DR;
    size--;
  }

  // wait until the last frame has fully shifted out before deasserting CS
  if (!spi_wait_set(&SPI1->SR, SPI_SR_TXE)) {
    spi_recover();
    return false;
  }
  if (!spi_wait_clear(&SPI1->SR, SPI_SR_BSY)) {
    spi_recover();
    return false;
  }

  spi_clear_status();
  return true;
}

void cs_enable(void) {
  GPIOA->BSRR = GPIO_BSRR_BR4;
}

void cs_disable(void) {
  GPIOA->BSRR = GPIO_BSRR_BS4;
}
