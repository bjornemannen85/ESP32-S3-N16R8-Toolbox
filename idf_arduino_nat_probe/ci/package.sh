#!/usr/bin/env bash
set -euo pipefail

echo "=== CREATE MERGED FIRMWARE ==="

python -m esptool --chip esp32s3 merge_bin \
  -o build/esp32-s3-v6.1-nat-merged.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 16MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/v61_arduino_nat_probe.bin

echo "=== VERIFY FIRMWARE ==="

ls -lh build/esp32-s3-v6.1-nat-merged.bin
sha256sum build/esp32-s3-v6.1-nat-merged.bin
