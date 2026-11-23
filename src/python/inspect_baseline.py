import argparse
import os
import struct
import zlib
from datetime import datetime, timezone
from pathlib import Path

PROFILE_FMT = "<IHH I ffff I QQ II"  # 56 bytes khớp chính xác struct C
PROFILE_SIZE = struct.calcsize(PROFILE_FMT)
MAGIC = 0x56544250
DEFAULT_PROFILE_PATH = (
    Path(r"C:\Temp\patient_baseline.bin")
    if os.name == "nt"
    else Path("/tmp/patient_baseline.bin")
)

def read_baseline_profile(path: str | Path) -> dict:
    with open(path, "rb") as f:
        raw = f.read(PROFILE_SIZE)
    if len(raw) != PROFILE_SIZE:
        raise ValueError(f"Kích thước tệp không hợp lệ: {len(raw)} bytes")

    fields = struct.unpack(PROFILE_FMT, raw)
    magic, ver, flags, subj_id, hr_base, hr_scale, spo2_base, rmssd_base, _, samples, ts, crc, _ = fields

    if magic != MAGIC:
        raise ValueError("Sai chữ ký định dạng VTBP")
    if (zlib.crc32(raw[:48]) & 0xFFFFFFFF) != crc:
        raise ValueError("Lỗi kiểm tra toàn vẹn CRC32")

    return {
        "anon_subject_id": subj_id,
        "version": ver,
        "clinically_seeded": bool(flags & 1),
        "hr_baseline_bpm": round(hr_base, 2),
        "hr_dev_scale": round(hr_scale, 2),
        "spo2_baseline_pct": round(spo2_base, 2),
        "rmssd_baseline_ms": round(rmssd_base, 2),
        "resting_samples_sec": samples,
        "last_updated_utc": datetime.fromtimestamp(ts, tz=timezone.utc).isoformat()
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Inspect an edge triage baseline profile.")
    parser.add_argument(
        "profile",
        nargs="?",
        type=Path,
        default=DEFAULT_PROFILE_PATH,
        help=f"profile file (default: {DEFAULT_PROFILE_PATH})",
    )
    args = parser.parse_args()
    print(read_baseline_profile(args.profile))
