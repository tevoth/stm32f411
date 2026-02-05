#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"


// syscalls.c
#include <sys/stat.h>
#include <errno.h>

int _close(int file) {
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;   // “character device”
    return 0;
}

int _isatty(int file) {
    (void)file;
    return 1;
}


int main(void) {
  uart_init();
  while(1) {
    printf("HELLO FROM STM32...\n"); 
  }
  return 0;
}
