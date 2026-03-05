# cmsis_max6675_sdcard_raw_ring_o1

MAX6675 logger that writes fixed-size records to raw SD sectors with O(1) startup recovery.

- No FAT filesystem is used.
- Data records are stored in a ring buffer over a reserved LBA range.
- Two metadata sectors store the current write head (`next_lba`, `next_seq`).
- On normal boot, firmware reads metadata only (constant-time startup).
- If metadata is missing/corrupt, firmware falls back to one full scan, then rewrites metadata.

## Build

```bash
cmake -S cmsis_max6675_sdcard_raw_ring_o1 -B /tmp/build_cmsis_max6675_sdcard_raw_ring_o1
cmake --build /tmp/build_cmsis_max6675_sdcard_raw_ring_o1
```

## Ring Configuration

Defaults are in `shared/sdcard_raw_ring_o1/inc/sd_raw_ring_o1.h`:

- `SD_RAW_RING_START_LBA = 32768`
- `SD_RAW_RING_SECTOR_COUNT = 4096`
- `SD_RAW_RING_META_START_LBA = SD_RAW_RING_START_LBA + SD_RAW_RING_SECTOR_COUNT`

Data ring uses LBAs:

- `[SD_RAW_RING_START_LBA, SD_RAW_RING_START_LBA + SD_RAW_RING_SECTOR_COUNT - 1]`

Metadata uses LBAs:

- `SD_RAW_RING_META_START_LBA`
- `SD_RAW_RING_META_START_LBA + 1`

Each data record consumes one 512-byte sector with:

- magic (`RLOG`)
- version
- payload fields (`seq`, `timestamp_ms`, `temp_c_x100`, `raw`, `status`)
- CRC32 over the full 512-byte record (with `crc32` field set to zero during calculation)

## O(1) Recovery Scheme

Metadata records store:

- magic/version
- `generation` counter
- `next_lba`
- `next_seq`
- CRC32

At init:

1. Read both metadata sectors.
2. Validate CRC.
3. Choose highest valid `generation`.
4. Resume writing from that `next_lba` and `next_seq`.

Each append:

1. Write one data sector.
2. Advance ring head.
3. Write metadata to the alternate metadata sector with `generation + 1`.

This preserves one previously valid metadata copy if power fails during metadata update.

## Readback

The host decoder reads only the data ring range (not metadata sectors):

`cmsis_max6675_sdcard_raw_ring_o1/tools/decode_raw_ring_windows.py`

Example (PowerShell as Administrator):

```powershell
python .\cmsis_max6675_sdcard_raw_ring_o1\tools\decode_raw_ring_windows.py `
  --device \\.\PhysicalDrive2 `
  --start-lba 32768 `
  --sector-count 4096 `
  --out .\max6675_ring.csv `
  --summary-only
```

## Metadata Debug Tool

Inspect the two metadata sectors used by O(1) boot:

`cmsis_max6675_sdcard_raw_ring_o1/tools/inspect_meta_windows.py`

Example (PowerShell as Administrator):

```powershell
python .\cmsis_max6675_sdcard_raw_ring_o1\tools\inspect_meta_windows.py `
  --device \\.\PhysicalDrive2 `
  --start-lba 32768 `
  --sector-count 4096
```

This prints both slot states (valid/invalid), `generation`, `next_lba`, `next_seq`, and which slot firmware would pick at boot.
