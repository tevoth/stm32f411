#include "spi2_sd.h"

#define SPI2_WAIT_LIMIT 100000U

static bool spi2_wait_set(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = SPI2_WAIT_LIMIT;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

static bool spi2_wait_clear(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = SPI2_WAIT_LIMIT;
  while (((*reg & mask) != 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) == 0U);
}

static void spi2_set_prescaler(uint32_t br_bits) {
  // RM: CR1 should be updated while SPI is disabled.
  SPI2->CR1 &= ~SPI_CR1_SPE;
  SPI2->CR1 = (SPI2->CR1 & ~SPI_CR1_BR_Msk) | br_bits;
  SPI2->CR1 |= SPI_CR1_SPE;
}

void spi2_sd_init(void) {
  // SPI2 lives on APB1, and this example uses PB12..PB15 pins.
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

  // Put SPI2 into a known reset state before reconfiguration.
  RCC->APB1RSTR |= RCC_APB1RSTR_SPI2RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI2RST;

  // PB13(SCK), PB14(MISO), PB15(MOSI) -> alternate-function mode.
  for (uint16_t pin = 13; pin <= 15; pin++) {
    GPIOB->MODER &= ~(3U << (pin * 2U));
    GPIOB->MODER |=  (2U << (pin * 2U));
    GPIOB->OTYPER &= ~(1U << pin);
    GPIOB->PUPDR &= ~(3U << (pin * 2U));
    GPIOB->OSPEEDR |= (3U << (pin * 2U)); // high speed edges for SPI.
  }

  // AF5 selects SPI2 on these pins.
  GPIOB->AFR[1] &= ~((0xFU << ((13U - 8U) * 4U)) |
                     (0xFU << ((14U - 8U) * 4U)) |
                     (0xFU << ((15U - 8U) * 4U)));
  GPIOB->AFR[1] |=  ((5U   << ((13U - 8U) * 4U)) |
                     (5U   << ((14U - 8U) * 4U)) |
                     (5U   << ((15U - 8U) * 4U)));

  // PB12 used as software chip-select for the SD card.
  GPIOB->MODER &= ~(3U << (12U * 2U));
  GPIOB->MODER |=  (1U << (12U * 2U));
  GPIOB->OTYPER &= ~(1U << 12U);
  GPIOB->PUPDR &= ~(3U << (12U * 2U));
  GPIOB->OSPEEDR |= (3U << (12U * 2U));
  spi2_sd_cs_high();

  // SPI mode 0 required for SD card SPI mode:
  // CPOL=0, CPHA=0, 8-bit, software NSS, master.
  uint32_t cr1 = 0U;
  cr1 |= SPI_CR1_MSTR;
  cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;
  // Start in very slow clock during card identification.
  cr1 |= SPI_CR1_BR_Msk; // /256
  SPI2->CR1 = cr1;
  SPI2->CR1 |= SPI_CR1_SPE;
}

void spi2_sd_set_slow_clock(void) {
  // /256 keeps the card init sequence comfortably under 400 kHz.
  spi2_set_prescaler(SPI_CR1_BR_Msk);
}

void spi2_sd_set_fast_clock(void) {
  // /4 on 16 MHz APB1 gives ~4 MHz. Safe starter speed for wiring/debug.
  spi2_set_prescaler(SPI_CR1_BR_0);
}

bool spi2_sd_transfer(uint8_t tx, uint8_t *rx) {
  if (rx == 0) {
    return false;
  }

  if (!spi2_wait_set(&SPI2->SR, SPI_SR_TXE)) {
    return false;
  }

  *(__IO uint8_t *)&SPI2->DR = tx;

  if (!spi2_wait_set(&SPI2->SR, SPI_SR_RXNE)) {
    return false;
  }

  *rx = *(__IO uint8_t *)&SPI2->DR;

  // Make sure the byte fully shifted before caller toggles CS.
  if (!spi2_wait_set(&SPI2->SR, SPI_SR_TXE)) {
    return false;
  }
  if (!spi2_wait_clear(&SPI2->SR, SPI_SR_BSY)) {
    return false;
  }

  return true;
}

void spi2_sd_cs_low(void) {
  GPIOB->BSRR = (1U << (12U + 16U));
}

void spi2_sd_cs_high(void) {
  GPIOB->BSRR = (1U << 12U);
}
