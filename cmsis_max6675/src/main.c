#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "spi.h"
#include "max6675.h"
#include "systick_msec_delay.h"
#include "system_init.h"

int main(void) {

  system_init();
  led_init();
  uart_init();
  spi_gpio_init();
  spi1_config();

  while(1) {
    uint16_t raw = 0;
    led_toggle();
    if (!max6675_read_raw(&raw)) {
      printf("MAX6675 read failed\n");
      systick_msec_delay(250);
      continue;
    }

    if (max6675_thermocouple_open(raw)) {
      printf("MAX6675 fault: thermocouple open (raw=0x%04X)\n", (unsigned)raw);
    } else {
      int32_t temp_c_x100 = max6675_temp_c_x100(raw);
      printf("MAX6675 temp: %" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
        temp_c_x100 / 100,
        temp_c_x100 % 100,
        (unsigned)raw);
    }

    // MAX6675 updates roughly every 220 ms.
    systick_msec_delay(250);
  }
  return 0;
}
