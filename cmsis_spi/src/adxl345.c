#include "adxl345.h"
#include <stdio.h>

void adxl_read(uint8_t address, uint8_t * rxdata)
{
  /*Set read operation*/
  address |= ADXL345_READ_OPERATION;

  /*Enable multi-byte*/
  address |= ADXL345_MULTI_BYTE_ENABLE;

  /*Pull cs line low to enable slave*/
  cs_enable();

  /*Send address*/
  spi1_transmit(&address,1);

  /*Read 6 bytes */
  spi1_receive(rxdata,6);

  /*Pull cs line high to disable slave*/
  cs_disable();
}

uint8_t adxl_read_reg(uint8_t address)
{
  uint8_t read_cmd = address | ADXL345_READ_OPERATION;
  uint8_t value;

  cs_enable();
  spi1_transmit(&read_cmd, 1);
  spi1_receive(&value, 1);
  cs_disable();

  return value;
}

uint8_t adxl_device_present(void)
{
  return (adxl_read_reg(ADXL345_REG_DEVID) == ADXL345_DEVICE_ID);
}


void adxl_write (uint8_t address, uint8_t value)
{
  uint8_t data[2];
  // single-byte write
  data[0] = address;

  // Place data into buffer
  data[1] = value;

  // Pull cs line low to enable slave
  cs_enable();

  // Transmit data and address
  spi1_transmit(data, 2);

  // Pull cs line high to disable slave
  cs_disable();
}

void adxl_init (void)
{
  // Enable SPI gpio
  spi_gpio_init();

  // Config SPI
  spi1_config();

  // Set data format range to +-4g and full range mode
  adxl_write(ADXL345_REG_DATA_FORMAT, ADXL345_FULL_RES | ADXL345_RANGE_4G);

  // Reset all bits
  adxl_write (ADXL345_REG_POWER_CTL, ADXL345_RESET);

  // Configure power control measure bit
  adxl_write (ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);

  // Check device
  uint8_t devid = adxl_read_reg(ADXL345_REG_DEVID);
  printf("ADXL345 DEVID read: 0x%02X (%s)\n", devid, 
    (devid == ADXL345_DEVICE_ID) ? "OK" : "BAD");
}
