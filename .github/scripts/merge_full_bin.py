#!/usr/bin/env python3
"""Print 'offset path' pairs (ascending) for esptool merge_bin.

Usage: merge_full_bin.py <env> <metadata.json>

<metadata.json> must be produced by `pio project metadata --json-output
-e <env>`, which includes extra.flash_images (bootloader, partition table,
boot_app0 - already resolved to their real on-disk paths, including
framework-package paths outside the project) and extra.application_offset.
It's generated explicitly rather than read from .pio/build/<env>/idedata.json,
since that file is an IDE-integration side effect that a plain `pio run`
does not reliably produce.
"""
import json
import sys

env, metadata_path = sys.argv[1], sys.argv[2]

with open(metadata_path) as f:
    extra = json.load(f)[env]["extra"]

images = [(img["offset"], img["path"]) for img in extra["flash_images"]]
images.append((extra["application_offset"], f".pio/build/{env}/firmware.bin"))
images.sort(key=lambda pair: int(pair[0], 16))

print(" ".join(f"{offset} {path}" for offset, path in images))
