#!/usr/bin/env python3
"""Decode MAX6675 raw-ring SD records from a Windows PhysicalDrive.

Run with Administrator privileges.
"""

from __future__ import annotations

import argparse
import csv
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List

SECTOR_SIZE = 512
MAGIC = 0x524C4F47  # 'RLOG'
VERSION = 1
PAYLOAD_SIZE = 15
HEADER_FMT = "<IHHIIiHBBI"
HEADER_SIZE = struct.calcsize(HEADER_FMT)


@dataclass
class Record:
    lba: int
    seq: int
    timestamp_ms: int
    temp_c_x100: int
    raw: int
    status: int


def parse_record(sector: bytes, lba: int) -> Record | None:
    if len(sector) != SECTOR_SIZE:
        return None

    magic, version, payload_size, seq, timestamp_ms, temp_c_x100, raw, status, _reserved0, stored_crc = struct.unpack(
        HEADER_FMT, sector[:HEADER_SIZE]
    )

    if magic != MAGIC or version != VERSION or payload_size != PAYLOAD_SIZE:
        return None

    mutable = bytearray(sector)
    mutable[24:28] = b"\x00\x00\x00\x00"
    calc_crc = zlib.crc32(mutable) & 0xFFFFFFFF
    if calc_crc != stored_crc:
        return None

    return Record(
        lba=lba,
        seq=seq,
        timestamp_ms=timestamp_ms,
        temp_c_x100=temp_c_x100,
        raw=raw,
        status=status,
    )


def iter_records(device_path: str, start_lba: int, sector_count: int) -> Iterable[Record]:
    with open(device_path, "rb", buffering=0) as f:
        f.seek(start_lba * SECTOR_SIZE)
        for i in range(sector_count):
            sector = f.read(SECTOR_SIZE)
            if len(sector) < SECTOR_SIZE:
                break
            rec = parse_record(sector, start_lba + i)
            if rec is not None:
                yield rec


def sort_ring_records(records: List[Record]) -> List[Record]:
    # Records are uniquely ordered by monotonic seq in firmware.
    return sorted(records, key=lambda r: r.seq)


def write_csv(records: Iterable[Record], output_csv: Path) -> None:
    output_csv.parent.mkdir(parents=True, exist_ok=True)
    with output_csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["seq", "timestamp_ms", "temp_c_x100", "temp_c", "raw", "status", "lba"])
        for r in records:
            temp_c = f"{r.temp_c_x100 / 100.0:.2f}"
            w.writerow([r.seq, r.timestamp_ms, r.temp_c_x100, temp_c, f"0x{r.raw:04X}", r.status, r.lba])


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Decode STM32 MAX6675 raw-ring records from a Windows PhysicalDrive into CSV."
    )
    p.add_argument(
        "--device",
        required=True,
        help=r"Windows raw drive path, e.g. \\.\PhysicalDrive2",
    )
    p.add_argument("--start-lba", type=int, default=32768, help="Ring start LBA (default: 32768)")
    p.add_argument("--sector-count", type=int, default=4096, help="Ring sector count (default: 4096)")
    p.add_argument("--out", required=True, type=Path, help="Output CSV path")
    p.add_argument(
        "--summary-only",
        action="store_true",
        help="Only print summary; do not write CSV",
    )
    return p


def main() -> int:
    args = build_arg_parser().parse_args()

    if args.start_lba < 0 or args.sector_count <= 0:
        print("error: --start-lba must be >= 0 and --sector-count must be > 0", file=sys.stderr)
        return 2

    try:
        valid_records = sort_ring_records(list(iter_records(args.device, args.start_lba, args.sector_count)))
    except PermissionError:
        print("error: permission denied opening device. Run as Administrator.", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"error: failed reading device: {exc}", file=sys.stderr)
        return 1

    print(f"valid records: {len(valid_records)}")
    if valid_records:
        print(f"seq range: {valid_records[0].seq} .. {valid_records[-1].seq}")
        print(f"lba range used by valid records: {valid_records[0].lba} .. {valid_records[-1].lba}")

    if args.summary_only:
        return 0

    if not valid_records:
        print("warning: no valid records found; CSV will contain only header", file=sys.stderr)

    write_csv(valid_records, args.out)
    print(f"wrote CSV: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
