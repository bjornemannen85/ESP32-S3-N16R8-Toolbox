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

echo "=== BUILD EXIT CODE: $result ==="
exit "$result"
