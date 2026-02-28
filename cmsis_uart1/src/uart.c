#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "uart.h"
#include "uart_helpers.h"

#define UART_BAUDRATE 115200
#define SYS_FREQ      16000000
#define APB2_CLK      SYS_FREQ
#define UART_WAIT_LIMIT 100000U

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static bool uart_write(int ch);

int __io_putchar(int ch) {
  return uart_write(ch) ? ch : -1;
}

int _write(int file, char *ptr, int len) {
    (void)file;
    if (len < 0) {
        // TODO: optionally set errno for invalid length input.
        return -1;
    }
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

  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR9_1);
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT9;
  // set to alternative function
  GPIOA->MODER &= ~GPIO_MODER_MODER9_0;
  GPIOA->MODER |=  GPIO_MODER_MODER9_1;
  // set PA9 to AF7
  GPIOA->AFR[1] &= ~(0xF << GPIO_AFRH_AFSEL9_Pos); // Clear all 4 bits for PA9
  GPIOA->AFR[1] |=  (0x7 << GPIO_AFRH_AFSEL9_Pos); // Set PA9 to AF7

  // enable clock to access uart1
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;

  uart_set_baudrate(APB2_CLK, UART_BAUDRATE);

  USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static bool uart_write(int ch) {
  if (!uart_wait_set_limit(&USART1->SR, USART_SR_TXE, UART_WAIT_LIMIT)) {
    return false;
  }

  USART1->DR = (ch & 0xFF);
  return true;
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate) {
  return uart_compute_bd(periph_clk, baudrate);
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate) {
  USART1->BRR = compute_uart_bd(periph_clk, baudrate);
}
