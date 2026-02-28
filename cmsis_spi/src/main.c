#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "adxl345.h"
#include "systick_msec_delay.h"
#include "system_init.h"

int16_t accel_x, accel_y, accel_z;

uint8_t data_buffer[6];

int main(void) {

  if (!system_init()) {
    while (1) {}
  }
  led_init();
  uart_init();
  if (!adxl_init()) {
    while (1) {
      printf("ADXL345 init failed\n");
      systick_msec_delay(500);
    }
  }

  while(1) {
    led_toggle();
    if (!adxl_read(ADXL345_REG_DATA_START, data_buffer)) {
      printf("ADXL345 read timeout\n");
      systick_msec_delay(100);
      continue;
    }

    // combine high and low bytes to form the accel data
    accel_x = (int16_t)((data_buffer[1] << 8) | data_buffer[0]);
    accel_y = (int16_t)((data_buffer[3] << 8) | data_buffer[2]);
    accel_z = (int16_t)((data_buffer[5] << 8) | data_buffer[4]);

    // convert raw data to milli-g (mg) using ADXL345 scale factor (~3.9 mg/LSB at +-4g)
    int32_t accel_x_mg = (int32_t)accel_x * 39 / 10;
    int32_t accel_y_mg = (int32_t)accel_y * 39 / 10;
    int32_t accel_z_mg = (int32_t)accel_z * 39 / 10;

    printf("accel_x : %6d (%" PRId32 " mg) accel_y : %6d (%" PRId32 " mg) accel_z : %6d (%" PRId32 " mg)\n",
      accel_x, accel_x_mg,
      accel_y, accel_y_mg,
      accel_z, accel_z_mg);

    systick_msec_delay(100);
  }
  return 0;
}
