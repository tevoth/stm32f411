#include "adxl345.h"
#include "spi_cs_xfer.h"
#include <stdio.h>

bool adxl_read(uint8_t address, uint8_t * rxdata)
{
  if (rxdata == 0U) {
    return false;
  }

  /*Set read operation*/
  address |= ADXL345_READ_OPERATION;

  /*Enable multi-byte*/
  address |= ADXL345_MULTI_BYTE_ENABLE;

  /*Pull cs line low to enable slave*/
  return spi_cs_transmit_receive(cs_enable, cs_disable, spi1_transmit, spi1_receive,
                                 &address, 1, rxdata, 6);
}

bool adxl_read_reg(uint8_t address, uint8_t *value)
{
  if (value == 0U) {
    return false;
  }

  uint8_t read_cmd = address | ADXL345_READ_OPERATION;

  return spi_cs_transmit_receive(cs_enable, cs_disable, spi1_transmit, spi1_receive,
                                 &read_cmd, 1, value, 1);
}

bool adxl_device_present(bool *present)
{
  if (present == 0U) {
    return false;
  }

  uint8_t devid = 0U;
  if (!adxl_read_reg(ADXL345_REG_DEVID, &devid)) {
    return false;
  }
  *present = (devid == ADXL345_DEVICE_ID);
  return true;
}


bool adxl_write(uint8_t address, uint8_t value)
{
  uint8_t data[2];
  // single-byte write
  data[0] = address;

  // Place data into buffer
  data[1] = value;

  // Transmit data and address
  return spi_cs_transmit(cs_enable, cs_disable, spi1_transmit, data, 2);
}

bool adxl_init(void)
{
  // Enable SPI gpio
  spi_gpio_init();

  // Config SPI
  spi1_config();

  // Set data format range to +-4g and full range mode
  if (!adxl_write(ADXL345_REG_DATA_FORMAT, ADXL345_FULL_RES | ADXL345_RANGE_4G)) {
    printf("ADXL345 init failed: DATA_FORMAT write timeout\n");
    return false;
  }

  // Reset all bits
  if (!adxl_write(ADXL345_REG_POWER_CTL, ADXL345_RESET)) {
    printf("ADXL345 init failed: POWER_CTL reset write timeout\n");
    return false;
  }

  // Configure power control measure bit
  if (!adxl_write(ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT)) {
    printf("ADXL345 init failed: POWER_CTL measure write timeout\n");
    return false;
  }

  // Check device
  uint8_t devid = 0U;
  if (!adxl_read_reg(ADXL345_REG_DEVID, &devid)) {
    printf("ADXL345 init failed: DEVID read timeout\n");
    return false;
  }
  printf("ADXL345 DEVID read: 0x%02X (%s)\n", (unsigned)devid, 
    (devid == ADXL345_DEVICE_ID) ? "OK" : "BAD");
  return (devid == ADXL345_DEVICE_ID);
}
