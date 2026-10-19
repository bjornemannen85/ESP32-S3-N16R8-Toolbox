# Cyberdeck V6.2 Full Access

Base: `v6.1-ultimate-pro` at `16e766162af5ad983ba4d37cf1021c7dec6bffe6`.
Development branch: `v6.2-full-access`. No stable branch is modified.

- Root web dashboard provides a searchable catalog with fields, explicit
  actions and downloads; every original page remains reachable.
- Touch: Tools -> All Programs invokes the same handlers used by web,
  with editable parameters, a symbol/multiline keyboard and paged output.
  UART, ADC, I2C and SD tiles lead to their installed controls.
- WiFi Setup/keyboard titles now have valid entries. The previous array had
  19 strings for 21 pages: opening WiFi Setup read past the array. A compile
  assertion prevents recurrence. Back navigation now uses the page history
  and the actual back-button rectangle.
- Existing NAT implementation, LCD/touch pins, touch conversion and NAT
  partition offsets are preserved. ESP-IDF 5.3.2 / Arduino 3.1.3 builds real
  lwIP NAPT; octal 8 MB PSRAM is enabled for N16R8.
- NAT build mounts the existing LittleFS partition using its actual label
  `storage`; no formatting is performed. An unformatted/corrupt filesystem
  still reports unavailable rather than erasing user data.
- BLE scanning is blocked while the file receiver runs. A stopped receiver
  clears the scanner's initialization flag so subsequent scans reinitialize.
- UART now excludes GPIO26-37 used by flash/octal PSRAM.
- A separate V6.2 Actions workflow runs source regression, native dispatcher
  and web logic tests, builds NAT firmware, verifies required configuration
  and the NAPT symbol, and uploads a V6.2-only merged image plus SHA-256.

Existing feature limits remain: no NFC/CC1101/IR drivers or GPIO output,
USB HID gated off, script storage/inspection without script execution.
Scans/diagnostics retain V6.1 synchronous behavior and can pause the UI.
Software tests and successful compilation do not validate physical hardware
or prove client internet forwarding; follow V62_FLASH_ANDROID.md on-device.
