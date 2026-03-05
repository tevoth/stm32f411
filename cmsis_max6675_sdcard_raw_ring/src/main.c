#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "led.h"
#include "max6675.h"
#include "sd_raw_ring.h"
#include "spi.h"
#include "systick_msec_delay.h"
#include "system_init.h"
#include "uart.h"

#define SAMPLE_PERIOD_MS 250U

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

  printf("\n=== MAX6675 + microSD raw ring logger ===\n");
  printf("SPI1: MAX6675, USART2: console, SPI2: microSD\n");
  printf("ring config: start_lba=%" PRIu32 ", sectors=%" PRIu32 "\n",
         (uint32_t)SD_RAW_RING_START_LBA,
         (uint32_t)SD_RAW_RING_SECTOR_COUNT);

  bool sd_logging_enabled = sd_raw_ring_init();
  if (sd_logging_enabled) {
    printf("microSD raw ring init OK (next_seq=%" PRIu32 ", next_lba=%" PRIu32 ")\n",
           sd_raw_ring_next_seq(),
           sd_raw_ring_next_lba());
  } else {
    printf("microSD init FAILED, continuing with UART only\n");
  }

  uint32_t seq = sd_raw_ring_next_seq();

  while (1) {
    led_toggle();

    uint16_t raw = 0U;
    max6675_status_t status = max6675_read_status(&raw);
    int32_t temp_c_x100 = 0;

    char line[96];
    int n = 0;

    if (status == MAX6675_STATUS_OK) {
      temp_c_x100 = max6675_temp_c_x100(raw);
      n = snprintf(line, sizeof(line),
                   "seq=%" PRIu32 ", temp=%" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
                   seq,
                   temp_c_x100 / 100,
                   temp_c_x100 % 100,
                   (unsigned)raw);
    } else if (status == MAX6675_STATUS_THERMOCOUPLE_OPEN) {
      n = snprintf(line, sizeof(line),
                   "seq=%" PRIu32 ", fault=open (raw=0x%04X)\n",
                   seq,
                   (unsigned)raw);
    } else if (status == MAX6675_STATUS_BUS_INVALID) {
      n = snprintf(line, sizeof(line),
                   "seq=%" PRIu32 ", fault=bus_invalid\n",
                   seq);
    } else {
      n = snprintf(line, sizeof(line),
                   "seq=%" PRIu32 ", fault=timeout\n",
                   seq);
    }

    if (n > 0) {
      printf("%s", line);
    }

    if (sd_logging_enabled) {
      sd_raw_ring_sample_t sample = {
        .seq = seq,
        .timestamp_ms = seq * SAMPLE_PERIOD_MS,
        .temp_c_x100 = temp_c_x100,
        .raw = raw,
        .status = (uint8_t)status,
      };

      if (!sd_raw_ring_append_sample(&sample)) {
        printf("microSD append failed; disabling SD logging\n");
        sd_logging_enabled = false;
      }
    }

    seq++;
    systick_msec_delay(SAMPLE_PERIOD_MS);
  }

  return 0;
}
