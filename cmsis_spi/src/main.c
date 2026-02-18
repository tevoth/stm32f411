#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "adxl345.h"
#include "systick_msec_delay.h"
#include "system_init.h"

int16_t accel_x, accel_y, accel_z;

uint8_t data_buffer[6];

int main(void) {

  system_init();
  led_init();
  uart_init();
  adxl_init();

  while(1) {
    led_toggle();
    adxl_read(ADXL345_REG_DATA_START, data_buffer);

    // combine high and low bytes to form the accel data
    accel_x = (int16_t)((data_buffer[1] << 8) | data_buffer[0]);
    accel_y = (int16_t)((data_buffer[3] << 8) | data_buffer[2]);
    accel_z = (int16_t)((data_buffer[5] << 8) | data_buffer[4]);

    // convert raw data to milli-g (mg) using ADXL345 scale factor (~3.9 mg/LSB at +-4g)
    int32_t accel_x_mg = (int32_t)accel_x * 39 / 10;
    int32_t accel_y_mg = (int32_t)accel_y * 39 / 10;
    int32_t accel_z_mg = (int32_t)accel_z * 39 / 10;

    printf("accel_x : %6d (%ld mg) accel_y : %6d (%ld mg) accel_z : %6d (%ld mg)\n",
      accel_x, accel_x_mg,
      accel_y, accel_y_mg,
      accel_z, accel_z_mg);

    systick_msec_delay(100);
  }
  return 0;
}
