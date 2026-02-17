#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "adxl345.h"
#include "systick_msec_delay.h"

int16_t accel_x, accel_y, accel_z;
double accel_x_g, accel_y_g, accel_z_g;

uint8_t data_buffer[6];

int main(void) {

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

    // convert raw data to g values
    accel_x_g = accel_x * 0.0078;
    accel_y_g = accel_y * 0.0078;
    accel_z_g = accel_z * 0.0078;

    printf("accel_x : %6d (0x%04X, %+0.3f g) accel_y : %6d (0x%04X, %+0.3f g) accel_z : %6d (0x%04X, %+0.3f g)\n",
      accel_x, (uint16_t)accel_x, accel_x_g,
      accel_y, (uint16_t)accel_y, accel_y_g,
      accel_z, (uint16_t)accel_z, accel_z_g);

    systick_msec_delay(100);
  }
  return 0;
}
