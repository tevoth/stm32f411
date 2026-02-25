#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "uart.h"

#define UART_BAUDRATE 115200
#define SYS_FREQ      16000000
#define APB1_CLK      SYS_FREQ
#define UART_WAIT_LIMIT 100000U

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static bool uart_write(int ch);
static bool uart_wait_set(volatile uint32_t *reg, uint32_t mask);

// Wait until status bits are set; false indicates timeout.
static bool uart_wait_set(volatile uint32_t *reg, uint32_t mask) {
  uint32_t timeout = UART_WAIT_LIMIT;
  while (((*reg & mask) == 0U) && (timeout-- > 0U)) {}
  return ((*reg & mask) != 0U);
}

int __io_putchar(int ch) {
  return uart_write(ch) ? ch : -1;
}

int _write(int file, char *ptr, int len) {
    (void)file;
    if ((ptr == 0) && (len > 0)) {
        // TODO: optionally set errno for invalid buffer input.
        return -1;
    }

    int written = 0;
    for (int i = 0; i < len; i++) {
        if (__io_putchar(*ptr++) < 0) {
            return (written > 0) ? written : -1;
        }
        written++;
    }
    return written;
}

void uart_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR2_1);
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT2;
  // set to alternative function
  GPIOA->MODER &= ~GPIO_MODER_MODER2_0;
  GPIOA->MODER |=  GPIO_MODER_MODER2_1;
  // set PA2 to AF7
  GPIOA->AFR[0] &= ~(0xF << GPIO_AFRL_AFSEL2_Pos); // Clear all 4 bits for PA2
  GPIOA->AFR[0] |=  (0x7 << GPIO_AFRL_AFSEL2_Pos); // Set PA2 to AF7

  // enable clock to access uart2
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

  uart_set_baudrate(APB1_CLK, UART_BAUDRATE);

  USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static bool uart_write(int ch) {
  if (!uart_wait_set(&USART2->SR, USART_SR_TXE)) {
    return false;
  }

  USART2->DR = (ch & 0xFF);
  return true;
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate) {
  uint16_t bd = ((periph_clk + (baudrate/2U))/baudrate);  
  return bd;
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate) {
  USART2->BRR = compute_uart_bd(periph_clk, baudrate);
}
