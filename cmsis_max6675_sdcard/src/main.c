#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "led.h"
#include "max6675.h"
#include "sd_raw_log.h"
#include "spi.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

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

  printf("\n=== MAX6675 + microSD logger demo ===\n");
  printf("SPI1: MAX6675, USART2: console, SPI2: microSD\n");

  bool sd_logging_enabled = sd_raw_log_init();
  if (sd_logging_enabled) {
    printf("microSD init OK (raw log start LBA=%" PRIu32 ")\n", (uint32_t)SD_RAW_LOG_START_LBA);
  } else {
    printf("microSD init FAILED, continuing with UART only\n");
  }

  uint32_t sample_index = 0U;

  while (1) {
    led_toggle();

    uint16_t raw = 0U;
    max6675_status_t status = max6675_read_status(&raw);

    char line[80];
    int n = 0;

    switch (status) {
      case MAX6675_STATUS_OK: {
        int32_t temp_c_x100 = max6675_temp_c_x100(raw);
        n = snprintf(line, sizeof(line),
          "sample=%" PRIu32 ", temp=%" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
          sample_index, temp_c_x100 / 100, temp_c_x100 % 100, (unsigned)raw);
      } break;

      case MAX6675_STATUS_THERMOCOUPLE_OPEN:
        n = snprintf(line, sizeof(line),
          "sample=%" PRIu32 ", fault=open (raw=0x%04X)\n",
          sample_index, (unsigned)raw);
        break;

      case MAX6675_STATUS_BUS_INVALID:
        n = snprintf(line, sizeof(line),
          "sample=%" PRIu32 ", fault=bus_invalid\n", sample_index);
        break;

      case MAX6675_STATUS_TIMEOUT:
      default:
        n = snprintf(line, sizeof(line),
          "sample=%" PRIu32 ", fault=timeout\n", sample_index);
        break;
    }

    if (n > 0) {
      // Always print to UART so you can monitor live behavior.
      printf("%s", line);

      // Also append to SD log when card is available.
      if (sd_logging_enabled) {
        if (!sd_raw_log_append_line(line)) {
          printf("microSD append failed; disabling SD logging\n");
          sd_logging_enabled = false;
        }
      }
    }

    // Force out partial sector periodically so logs appear sooner on card.
    if (sd_logging_enabled && (sample_index % 16U == 15U)) {
      if (!sd_raw_log_flush_pending()) {
        printf("microSD flush failed; disabling SD logging\n");
        sd_logging_enabled = false;
      }
    }

    sample_index++;
    // MAX6675 updates roughly every 220 ms.
    systick_msec_delay(250);
  }

  return 0;
}
