#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"

#define UART_BAUDRATE 115200
#define SYS_FREQ      16000000
#define APB1_CLK      SYS_FREQ

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static void uart_write(int ch);

int __io_putchar(int ch) {
  uart_write(ch);
  return ch;
}

int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        __io_putchar(*ptr++);
    }
    return len;
}

void uart_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR2_1);
  // set to alternative function
  GPIOA->MODER &= ~GPIO_MODER_MODER2_0;
  GPIOA->MODER |=  GPIO_MODER_MODER2_1;
  // set PA2 to AF7
  GPIOA->AFR[0] &= ~(0xF << GPIO_AFRL_AFSEL2_Pos); // Clear all 4 bits for PA2
  GPIOA->AFR[0] |=  (0x7 << GPIO_AFRL_AFSEL2_Pos); // Set PA2 to AF7

  // enable clock to access uart2
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  uart_set_baudrate(APB1_CLK, UART_BAUDRATE);

  USART2->CR1 |= USART_CR1_TE;
  USART2->CR1 |= USART_CR1_UE;
}

static void uart_write(int ch) {
  while (!(USART2->SR & USART_SR_TXE)){}

  USART2->DR = (ch & 0xFF);
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate) {
  uint16_t bd = ((periph_clk + (baudrate/2U))/baudrate);  
  return bd;
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate) {
  USART2->BRR = compute_uart_bd(periph_clk, baudrate);
}
