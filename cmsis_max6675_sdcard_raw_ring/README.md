# cmsis_max6675_sdcard_raw_ring

MAX6675 logger that writes fixed-size binary records directly to raw SD sectors.

- No FAT filesystem is used.
- Logging uses a ring buffer over a reserved LBA range.
- On boot, firmware scans the reserved range, finds the newest valid record, and continues appending.

## Build

```bash
cmake -S cmsis_max6675_sdcard_raw_ring -B /tmp/build_cmsis_max6675_sdcard_raw_ring
cmake --build /tmp/build_cmsis_max6675_sdcard_raw_ring
```

## Ring Configuration

Defaults are in `shared/sdcard_raw_ring/inc/sd_raw_ring.h`:

- `SD_RAW_RING_START_LBA = 32768`
- `SD_RAW_RING_SECTOR_COUNT = 4096`

Each log record consumes one 512-byte sector with:

- magic (`RLOG`)
- version
- payload fields (`seq`, `timestamp_ms`, `temp_c_x100`, `raw`, `status`)
- CRC32 over the full 512-byte record (with `crc32` field set to zero during calculation)

When the last sector in the reserved range is reached, writing wraps to `SD_RAW_RING_START_LBA` and overwrites oldest records.

## Readback

### Option A: host raw-device decoder

Read the reserved LBA range and decode `RLOG` records.

- Linux: read `/dev/sdX` with root permissions.
- Windows: read `\\.\PhysicalDriveN` with admin permissions.

A decoder can scan all sectors, validate CRC, sort by `seq`, then export CSV.

Windows decoder script:

`cmsis_max6675_sdcard_raw_ring/tools/decode_raw_ring_windows.py`

Example (PowerShell as Administrator):

```powershell
python .\cmsis_max6675_sdcard_raw_ring\tools\decode_raw_ring_windows.py `
  --device \\.\PhysicalDrive2 `
  --start-lba 32768 `
  --sector-count 4096 `
  --out .\max6675_ring.csv
```

To only inspect validity and sequence range:

```powershell
python .\cmsis_max6675_sdcard_raw_ring\tools\decode_raw_ring_windows.py `
  --device \\.\PhysicalDrive2 `
  --start-lba 32768 `
  --sector-count 4096 `
  --out .\max6675_ring.csv `
  --summary-only
```

### Option B: firmware export command

Add a UART command in firmware that scans and streams valid records as CSV.

This is often easier for end users because it avoids direct raw-disk access on host OS.
