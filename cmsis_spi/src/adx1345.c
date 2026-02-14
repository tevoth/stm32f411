#include "adx1345.h"

void adxl_read_bytes(uint8_t address, uint8_t *rxdata, uint8_t len)
{
  uint8_t read_cmd = address | ADXL345_READ_OPERATION;

  if (len > 1) {
    read_cmd |= ADXL345_MULTI_BYTE_ENABLE;
  }

  cs_enable();
  spi1_transmit(&read_cmd, 1);
  spi1_receive(rxdata, len);
  cs_disable();
}

void adxl_read(uint8_t address, uint8_t * rxdata)
{
  adxl_read_bytes(address, rxdata, 6);
}

uint8_t adxl_read_reg(uint8_t address)
{
  uint8_t value;
  adxl_read_bytes(address, &value, 1);
  return value;
}

uint8_t adxl_device_present(void)
{
  return (adxl_read_reg(ADXL345_REG_DEVID) == ADXL345_DEVICE_ID);
}


void adxl_write (uint8_t address, uint8_t value)
{
  uint8_t data[2];
  // Enable multi-byte, place address into buffer
  // data[0] = address|ADXL345_MULTI_BYTE_ENABLE;
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
  /*Enable SPI gpio*/
  spi_gpio_init();

  /*Config SPI*/
  spi1_config();

  /*Set data format range to +-4g*/
  adxl_write (ADXL345_REG_DATA_FORMAT, ADXL345_RANGE_4G);

  /*Reset all bits*/
  adxl_write (ADXL345_REG_POWER_CTL, ADXL345_RESET);

  /*Configure power control measure bit*/
  adxl_write (ADXL345_REG_POWER_CTL, ADXL345_MEASURE_BIT);
}
