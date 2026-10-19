# Cyberdeck V6.2 Full Access — Android

This is a V6.2 development build based on V6.1 commit
`16e766162af5ad983ba4d37cf1021c7dec6bffe6`.
A successful Actions build is not a hardware test.

Download the artifact **cyberdeck-v6.2-full-access-nat-firmware** from the
**Cyberdeck V6.2 Full Access NAT** workflow on `v6.2-full-access`.
Extract it; do not flash the ZIP. Verify the BIN against SHA256SUMS.txt.

For ESP32-S3 N16R8 / ES3C28P only, select ESP32-S3, 16 MB flash,
DIO, 80 MHz in the Android flasher and write
`esp32-s3-v6.2-full-access-nat-merged.bin` at **0x0**.
Keep the working V6.1 BIN for rollback. Do not erase the entire flash;
this image excludes NVS and LittleFS, so existing credentials and files can
survive when flashing the unchanged V6.1 NAT partition layout.

After boot:
- Settings -> WiFi Setup: leave open for at least 30 seconds; test all four
  input fields, symbol keyboard, DONE and BACK repeatedly.
- Tools -> All Programs: every installed API action has editable fields,
  an explicit RUN button and a paged result. Touch is locally authorized.
- UART, I2C, GPIO/ADC and SD home tiles open their installed controls.
- Open the AP address (normally http://192.168.4.1/) for Full Access.
  Enter the admin token from Settings. Original web pages remain linked.
- WiFi NAT: choose POST /wifi/nat, enabled=1. Verify from an Android client
  connected to the Cyberdeck AP that DNS and internet forwarding work
  while the Cyberdeck STA is connected to your upstream network.
- Verify BLE scanning, BLE receiver start/code/stop, UART start/stop/RX/TX,
  script file operations and manual ADC/SD functions with your hardware.
- Verify PSRAM reports 8 MB and that display/touch still work.

NFC/PN532, CC1101, IR and GPIO output were not implemented in the base
source. Native USB HID remains gated off. Full Access exposes existing
programs; it does not implement these missing drivers or execute scripts.

The adapter executes the original handlers on the same loop task as web.
Web actions retain the existing admin-token authentication and validation.
Touch previews retain up to 48 KiB; full responses can be downloaded on web.
WiFi/BLE scans are synchronous as in V6.1 and can briefly pause the UI.
