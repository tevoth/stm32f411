#pragma once

#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#define ADXL345_REG_DEVID (0x00)
#define ADXL345_REG_DATA_FORMAT (0x31)
#define ADXL345_REG_POWER_CTL (0x2D)
#define ADXL345_REG_DATA_START (0x32)
#define ADXL345_RANGE_4G (0x01)
#define ADXL345_RESET (0x00)
#define ADXL345_MEASURE_BIT (0x08)
#define ADXL345_MULTI_BYTE_ENABLE (0x40)
#define ADXL345_READ_OPERATION (0x80)
#define ADXL345_DEVICE_ID (0xE5)
#define ADXL345_FULL_RES (1U << 3)

bool adxl_init(void);
bool adxl_read(uint8_t address, uint8_t * rxdata);
uint8_t adxl_read_reg(uint8_t address);
uint8_t adxl_device_present(void);
bool adxl_write(uint8_t address, uint8_t value);
