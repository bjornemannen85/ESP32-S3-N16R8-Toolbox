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
