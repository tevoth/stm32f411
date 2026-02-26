#include "fatfs/ff.h"
#include "fatfs/diskio.h"

#include "sdcard_spi.h"

// Single physical drive for this demo.
#define DEV_SD 0U

DSTATUS disk_initialize(BYTE pdrv) {
  if (pdrv != DEV_SD) {
    return STA_NOINIT;
  }

  return sdcard_spi_init() ? 0U : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
  if (pdrv != DEV_SD) {
    return STA_NOINIT;
  }

  // Demo assumes card remains inserted after successful init.
  return 0U;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
  if ((pdrv != DEV_SD) || (buff == 0) || (count == 0U)) {
    return RES_PARERR;
  }

  for (UINT i = 0; i < count; i++) {
    if (!sdcard_spi_read_block((uint32_t)sector + i, &buff[i * 512U])) {
      return RES_ERROR;
    }
  }

  return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
  if ((pdrv != DEV_SD) || (buff == 0) || (count == 0U)) {
    return RES_PARERR;
  }

  for (UINT i = 0; i < count; i++) {
    if (!sdcard_spi_write_block((uint32_t)sector + i, &buff[i * 512U])) {
      return RES_ERROR;
    }
  }

  return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
  if (pdrv != DEV_SD) {
    return RES_PARERR;
  }

  switch (cmd) {
    case CTRL_SYNC:
      return RES_OK;

    case GET_SECTOR_COUNT:
      if (buff == 0) {
        return RES_PARERR;
      }
      *(LBA_t *)buff = 0x00080000U;
      return RES_OK;

    case GET_SECTOR_SIZE:
      if (buff == 0) {
        return RES_PARERR;
      }
      *(WORD *)buff = 512U;
      return RES_OK;

    case GET_BLOCK_SIZE:
      if (buff == 0) {
        return RES_PARERR;
      }
      *(DWORD *)buff = 1U;
      return RES_OK;

    default:
      return RES_PARERR;
  }
}
