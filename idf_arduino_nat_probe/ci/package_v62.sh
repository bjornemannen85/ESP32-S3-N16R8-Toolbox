#!/usr/bin/env bash
set -euo pipefail
# Keep the known-good V6.1 NAT partition layout and flash parameters.
python - <<'PY'
from pathlib import Path
cfg=Path('sdkconfig').read_text()
for flag in ('CONFIG_LWIP_IP_FORWARD=y','CONFIG_LWIP_IPV4_NAPT=y','CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y','CONFIG_SPIRAM=y','CONFIG_SPIRAM_MODE_OCT=y'):
    assert flag in cfg, f'Required configuration missing: {flag}'
PY
xtensa-esp32s3-elf-nm -g build/esp-idf/lwip/liblwip.a | grep -E ' [TW] ip_napt_enable$'
mkdir -p build/v62-dist
python -m esptool --chip esp32s3 merge_bin \
  -o build/v62-dist/esp32-s3-v6.2-full-access-nat-merged.bin \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/v61_arduino_nat_probe.bin
cp ../V62_FLASH_ANDROID.md build/v62-dist/README-ANDROID.md
cp ../WOLFDECK_V62.md build/v62-dist/WOLFDECK-NOTES.md
cp ../assets/wolfdeck/manifest.json build/v62-dist/ASSETS.json
cp ../assets/wolfdeck/icons/manifest.json build/v62-dist/APP-ICONS.json
cp ../V62_WORKBENCH.md build/v62-dist/WORKBENCH-NOTES.md
mkdir -p build/v62-dist/SCRIPTS
cp -R ../scripts/. build/v62-dist/SCRIPTS/
git rev-parse HEAD > build/v62-dist/SOURCE_COMMIT.txt
(cd build/v62-dist && sha256sum esp32-s3-v6.2-full-access-nat-merged.bin > SHA256SUMS.txt)
python -m esptool --chip esp32s3 image_info build/v61_arduino_nat_probe.bin
