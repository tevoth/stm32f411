#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"

#define UART_BAUDRATE 115200
#define SYS_FREQ      16000000
#define APB2_CLK      SYS_FREQ

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

  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR9_1);
  // set to alternative function
  GPIOA->MODER &= ~GPIO_MODER_MODER9_0;
  GPIOA->MODER |=  GPIO_MODER_MODER9_1;
  // set PA9 to AF7
  GPIOA->AFR[1] &= ~(0xF << GPIO_AFRH_AFSEL9_Pos); // Clear all 4 bits for PA9
  GPIOA->AFR[1] |=  (0x7 << GPIO_AFRH_AFSEL9_Pos); // Set PA9 to AF7

  // enable clock to access uart1
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

  uart_set_baudrate(APB2_CLK, UART_BAUDRATE);

  USART1->CR1 |= USART_CR1_TE;
  USART1->CR1 |= USART_CR1_UE;
}

static void uart_write(int ch) {
  while (!(USART1->SR & USART_SR_TXE)){}

  USART1->DR = (ch & 0xFF);
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate) {
  uint16_t bd = ((periph_clk + (baudrate/2U))/baudrate);  
  return bd;
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate) {
  USART1->BRR = compute_uart_bd(periph_clk, baudrate);
}
