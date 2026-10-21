#!/usr/bin/env python3
"""Package a built NMC firmware into a .nmcfw image.

Layout (128-byte header, little endian) - must match firmware/include/fw_update.h:
  0  magic "NMCF" | 4 header_version=1 | 8 variant[16] | 24 version[32]
  56 payload_len u32 | 60 payload_sha256[32] | 92..127 reserved
Payload = tar.gz of the install tree (binary + web UI + MIBs).
Usage: fwpack.py --variant GVS --version 1.4.0 --input build/install --output dist/nmc-GVS-1.4.0.nmcfw
"""
import argparse
import hashlib
import io
import json
import struct
import tarfile
import time
from pathlib import Path


def build_payload(src: Path) -> bytes:
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz", format=tarfile.PAX_FORMAT) as tar:
        for p in sorted(src.rglob("*")):           # sorted + fixed mtime = reproducible
            info = tar.gettarinfo(str(p), arcname=str(p.relative_to(src)))
            info.mtime, info.uid, info.gid, info.uname, info.gname = 0, 0, 0, "root", "root"
            if p.is_file():
                with open(p, "rb") as f:
                    tar.addfile(info, f)
            else:
                tar.addfile(info)
    return buf.getvalue()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--variant", required=True, choices=["GVS", "GVL", "GVXL", "GVXL-NAM"])
    ap.add_argument("--version", required=True)
    ap.add_argument("--input", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    a = ap.parse_args()

    payload = build_payload(a.input)
    digest = hashlib.sha256(payload).digest()
    hdr = struct.pack("<4sB3s16s32sI32s", b"NMCF", 1, b"\0" * 3,
                      a.variant.encode(), a.version.encode(), len(payload), digest)
    hdr = hdr.ljust(128, b"\0")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(hdr + payload)
    manifest = {"variant": a.variant, "version": a.version, "file": a.output.name,
                "payload_sha256": digest.hex(), "image_sha256": hashlib.sha256(hdr + payload).hexdigest(),
                "size": len(hdr) + len(payload), "built_at": int(time.time())}
    a.output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest))


if __name__ == "__main__":
    main()
