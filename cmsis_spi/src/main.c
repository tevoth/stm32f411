#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "led.h"
#include "uart.h"
#include "adx1345.h"

int16_t accel_x, accel_y, accel_z;
double accel_x_g, accel_y_g, accel_z_g;

uint8_t data_buffer[6];

int main(void) {
  uint8_t devid;

  led_init();
  uart_init();
  adxl_init();

  devid = adxl_read_reg(ADXL345_REG_DEVID);
  printf("ADXL345 DEVID read: 0x%02X (%s)\n", devid, 
    (devid == ADXL345_DEVICE_ID) ? "OK" : "BAD");
  while(1) {
    led_toggle();
    adxl_read(ADXL345_REG_DATA_START, data_buffer);

    // combine high and low bytes to form the accel data
    accel_x = (int16_t)((data_buffer[1] << 8) | data_buffer[0]);
    accel_y = (int16_t)((data_buffer[3] << 8) | data_buffer[2]);
    accel_z = (int16_t)((data_buffer[5] << 8) | data_buffer[4]);

    // convert raw data to g values;     
    float accel_g_x = accel_x * 0.0078f;
    float accel_g_y = accel_y * 0.0078f;
    float accel_g_z = accel_z * 0.0078f;

    printf("accel_x: %6d (0x%04X) accel_y: %6d (0x%04X) accel_z: %6d (0x%04X)\n",
      accel_x, (uint16_t)accel_x,
      accel_y, (uint16_t)accel_y,
      accel_z, (uint16_t)accel_z);

    for (int i = 0; i < 100000; i++) {}
  }
  return 0;
}
