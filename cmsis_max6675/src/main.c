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
    max6675_status_t status = max6675_read_status(&raw);

    switch (status) {
      case MAX6675_STATUS_OK: {
        int32_t temp_c_x100 = max6675_temp_c_x100(raw);
        printf("MAX6675 temp: %" PRId32 ".%02" PRId32 " C (raw=0x%04X)\n",
          temp_c_x100 / 100,
          temp_c_x100 % 100,
          (unsigned)raw);
      } break;

      case MAX6675_STATUS_THERMOCOUPLE_OPEN:
        printf("MAX6675 fault: thermocouple open (raw=0x%04X)\n", (unsigned)raw);
        break;

      case MAX6675_STATUS_BUS_INVALID:
        printf("MAX6675 fault: invalid SPI frame (sensor disconnected or bus noise)\n");
        break;

      case MAX6675_STATUS_TIMEOUT:
      default:
        printf("MAX6675 fault: SPI timeout\n");
        break;
    }

    // MAX6675 updates roughly every 220 ms.
    systick_msec_delay(250);
  }
  return 0;
}
