#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "fatfs_log.h"
#include "led.h"
#include "max6675.h"
#include "spi.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

#define LOG_LINE_BUF_SZ 96U

int main(void) {
  if (!system_init()) {
    while (1) {
      printf("system_init failed\n");
      systick_msec_delay(500);
    }
  }
  led_init();
  uart_init();
  spi_gpio_init();
  spi1_config();

  printf("\n=== MAX6675 + FAT32 microSD logger demo ===\n");
  printf("SPI1: MAX6675, USART2: console, SPI2: microSD(FAT32)\n");

  bool sd_logging_enabled = fatfs_log_init();
  if (sd_logging_enabled) {
    printf("microSD FAT32 log init OK\n");
  } else {
    printf("microSD FAT32 init FAILED, continuing with UART only\n");
  }

  uint32_t sample_index = 0U;

  while (1) {
    led_toggle();

    uint16_t raw = 0U;
    max6675_status_t status = max6675_read_status(&raw);

    char uart_line[LOG_LINE_BUF_SZ];
    char csv_line[LOG_LINE_BUF_SZ];
    int uart_n = 0;
    int csv_n = 0;

    switch (status) {
      case MAX6675_STATUS_OK: {
        int32_t temp_c_x100 = max6675_temp_c_x100(raw);
        uart_n = snprintf(uart_line, sizeof(uart_line),
          "sample=%" PRIu32 ", temp=%" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
          sample_index, temp_c_x100 / 100, temp_c_x100 % 100, (unsigned)raw);
        csv_n = snprintf(csv_line, sizeof(csv_line),
          "%" PRIu32 ",ok,%" PRId32 ",0x%04X\r\n",
          sample_index, temp_c_x100, (unsigned)raw);
      } break;

      case MAX6675_STATUS_THERMOCOUPLE_OPEN:
        uart_n = snprintf(uart_line, sizeof(uart_line),
          "sample=%" PRIu32 ", fault=open (raw=0x%04X)\n",
          sample_index, (unsigned)raw);
        csv_n = snprintf(csv_line, sizeof(csv_line),
          "%" PRIu32 ",open,,0x%04X\r\n",
          sample_index, (unsigned)raw);
        break;

      case MAX6675_STATUS_BUS_INVALID:
        uart_n = snprintf(uart_line, sizeof(uart_line),
          "sample=%" PRIu32 ", fault=bus_invalid\n", sample_index);
        csv_n = snprintf(csv_line, sizeof(csv_line),
          "%" PRIu32 ",bus_invalid,,\r\n", sample_index);
        break;

      case MAX6675_STATUS_TIMEOUT:
      default:
        uart_n = snprintf(uart_line, sizeof(uart_line),
          "sample=%" PRIu32 ", fault=timeout\n", sample_index);
        csv_n = snprintf(csv_line, sizeof(csv_line),
          "%" PRIu32 ",timeout,,\r\n", sample_index);
        break;
    }

    if (uart_n > 0) {
      // Always print to UART so you can monitor live behavior.
      printf("%s", uart_line);

      // Also append to SD log when card is available.
      if (sd_logging_enabled) {
        if ((csv_n <= 0) || !fatfs_log_append_line(csv_line)) {
          printf("microSD FAT32 append failed; disabling SD logging\n");
          sd_logging_enabled = false;
        }
      }
    }

    sample_index++;
    // MAX6675 updates roughly every 220 ms.
    systick_msec_delay(250);
  }

  return 0;
}
