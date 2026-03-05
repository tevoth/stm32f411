#!/usr/bin/env python3
"""Inspect O(1) raw-ring metadata sectors from a Windows PhysicalDrive.

Run with Administrator privileges.
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from dataclasses import dataclass

SECTOR_SIZE = 512
META_MAGIC = 0x524D4554  # 'RMET'
META_VERSION = 1
META_PAYLOAD_SIZE = 8
META_HEADER_FMT = "<IHHIIII"
META_HEADER_SIZE = struct.calcsize(META_HEADER_FMT)


@dataclass
class MetaRecord:
    slot: int
    lba: int
    generation: int
    next_lba: int
    next_seq: int


def parse_meta_sector(sector: bytes, slot: int, lba: int) -> tuple[MetaRecord | None, str]:
    if len(sector) != SECTOR_SIZE:
        return None, "short read"

    magic, version, payload_size, generation, next_lba, next_seq, stored_crc = struct.unpack(
        META_HEADER_FMT, sector[:META_HEADER_SIZE]
    )

    if magic != META_MAGIC:
        return None, f"bad magic: 0x{magic:08X}"
    if version != META_VERSION:
        return None, f"bad version: {version}"
    if payload_size != META_PAYLOAD_SIZE:
        return None, f"bad payload_size: {payload_size}"

    mutable = bytearray(sector)
    mutable[20:24] = b"\x00\x00\x00\x00"
    calc_crc = zlib.crc32(mutable) & 0xFFFFFFFF
    if calc_crc != stored_crc:
        return None, f"crc mismatch: stored=0x{stored_crc:08X} calc=0x{calc_crc:08X}"

    return (
        MetaRecord(
            slot=slot,
            lba=lba,
            generation=generation,
            next_lba=next_lba,
            next_seq=next_seq,
        ),
        "ok",
    )


def read_sector(f, lba: int) -> bytes:
    f.seek(lba * SECTOR_SIZE)
    return f.read(SECTOR_SIZE)


def choose_newest(a: MetaRecord | None, b: MetaRecord | None) -> MetaRecord | None:
    if a is None and b is None:
        return None
    if a is None:
        return b
    if b is None:
        return a

    # Same signed-wrap comparison style as firmware.
    diff = ((b.generation - a.generation) + (1 << 31)) % (1 << 32) - (1 << 31)
    return b if diff > 0 else a


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Inspect STM32 O(1) raw-ring metadata sectors from a Windows PhysicalDrive."
    )
    p.add_argument(
        "--device",
        required=True,
        help=r"Windows raw drive path, e.g. \\.\PhysicalDrive2",
    )
    p.add_argument("--start-lba", type=int, default=32768, help="Ring start LBA (default: 32768)")
    p.add_argument("--sector-count", type=int, default=4096, help="Ring sector count (default: 4096)")
    p.add_argument(
        "--meta-start-lba",
        type=int,
        default=None,
        help="Metadata slot 0 LBA (default: start-lba + sector-count)",
    )
    return p


def main() -> int:
    args = build_arg_parser().parse_args()

    if args.start_lba < 0 or args.sector_count <= 0:
        print("error: --start-lba must be >= 0 and --sector-count must be > 0", file=sys.stderr)
        return 2

    meta_start_lba = args.meta_start_lba
    if meta_start_lba is None:
        meta_start_lba = args.start_lba + args.sector_count

    if meta_start_lba < 0:
        print("error: --meta-start-lba must be >= 0", file=sys.stderr)
        return 2

    try:
        with open(args.device, "rb", buffering=0) as f:
            rec0, reason0 = parse_meta_sector(read_sector(f, meta_start_lba + 0), 0, meta_start_lba + 0)
            rec1, reason1 = parse_meta_sector(read_sector(f, meta_start_lba + 1), 1, meta_start_lba + 1)
    except PermissionError:
        print("error: permission denied opening device. Run as Administrator.", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"error: failed reading device: {exc}", file=sys.stderr)
        return 1

    print(f"ring start_lba={args.start_lba} sector_count={args.sector_count}")
    print(f"meta slots at LBA {meta_start_lba} and {meta_start_lba + 1}")

    for rec, reason, slot_lba in ((rec0, reason0, meta_start_lba), (rec1, reason1, meta_start_lba + 1)):
        if rec is None:
            print(f"slot@{slot_lba}: INVALID ({reason})")
        else:
            print(
                f"slot@{slot_lba}: VALID gen={rec.generation} next_lba={rec.next_lba} next_seq={rec.next_seq}"
            )

    newest = choose_newest(rec0, rec1)
    if newest is None:
        print("boot decision: no valid metadata, firmware will fall back to full ring scan")
        return 0

    print(
        f"boot decision: use slot={newest.slot} (lba={newest.lba}) "
        f"gen={newest.generation} next_lba={newest.next_lba} next_seq={newest.next_seq}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

