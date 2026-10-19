#!/usr/bin/env bash
set +e

idf.py build
result=$?

echo "=== SSL SYMBOL DIAGNOSTICS ==="

LIB="build/esp-idf/espressif__arduino-esp32/libespressif__arduino-esp32.a"

if [ -f "$LIB" ]; then
  xtensa-esp32-elf-nm -C "$LIB" |
    grep -E 'ssl_init|start_ssl_client|stop_ssl_socket|send_ssl_data|send_net_data' |
    head -60 || true
else
  echo "Arduino library not found: $LIB"
fi

echo "=== SSL OBJECT SYMBOLS ==="
OBJ="build/esp-idf/espressif__arduino-esp32/CMakeFiles/__idf_espressif__arduino-esp32.dir/libraries/NetworkClientSecure/src/ssl_client.cpp.obj"

if [ -f "$OBJ" ]; then
  xtensa-esp32-elf-nm -C "$OBJ" |
    grep -E 'ssl_init|start_ssl_client|stop_ssl_socket|send_ssl_data|send_net_data' || true
else
  echo "SSL object not found"
fi

echo "=== SSL SOURCE CONDITIONS ==="
SRC="managed_components/espressif__arduino-esp32/libraries/NetworkClientSecure/src/ssl_client.cpp"

grep -nE '^#(if|ifdef|ifndef|else|elif|endif)' "$SRC" | head -65 || true

echo "=== BUILD EXIT CODE: $result ==="
exit "$result"


if [ "$result" -eq 0 ]; then
  echo "=== CREATE MERGED FIRMWARE ==="

  python -m esptool --chip esp32s3 merge_bin     -o build/esp32-s3-v6.1-nat-merged.bin     --flash_mode dio     --flash_freq 80m     --flash_size 16MB     0x0 build/bootloader/bootloader.bin     0x8000 build/partition_table/partition-table.bin     0x10000 build/v61_arduino_nat_probe.bin

  result=$?

  if [ "$result" -eq 0 ]; then
    sha256sum build/esp32-s3-v6.1-nat-merged.bin
  fi
fi

exit "$result"
