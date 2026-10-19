# WOLF DECK V6.2 Workbench

This revision extends the icon firmware on `v6.2-full-access`. V6.0/V6.1
branches and existing firmware deliveries are untouched.

## Navigation and applications

Touch lists, scan lists, results and script instructions now have a draggable
right-side scrollbar. Arrow paging remains. The rightmost 18 pixels are
reserved for dragging; it cannot activate an underlying program. Large result
scrolling uses constant-memory line scanning, not an index per line.
Web uses persistent, visible right-side scrollbars and bounded result panels.

Files / Scripts contains native BadUSB and UART libraries. Every built-in file
has source content, instructions, a pinned GitHub origin and license metadata.
Touch detail view starts on HELP. CODE toggles the source; ARM starts native
HID without running a script; RUN executes the selected macro; STOP releases
HID keys/buttons and cancels the macro. A status line updates without redrawing
the wallpaper. Arduino .ino reference sketches are for a separate test board.

Web `/apps` contains app guides and module controls; `/apps/scripts` provides
file selection, instructions, validation, editable copies, manual run/stop and
download. Existing tool APIs remain available from both touch and web, with
useful UART/ADC/network defaults. Historical feature endpoints now use live
capability data instead of hardcoded NAT/BLE/HID staging flags.

## Script folders and instructions

- `/badusb`: five original Flipper Zero demonstration scripts, GPL-3.0-only,
  pinned to commit `3fb1d51727733d2007abc5ec636a89328857803e`.
- `/uart`: four Arduino public-domain communication examples pinned to
  `50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a`, plus five original native macros
  explaining/testing their workflows. The .ino files run on a separate board.
- Each file has an accompanying `.md` instruction document and source metadata
  in `scripts/manifest.json`. UART macros start only after UART RX/TX/baud is
  configured. There is no Python/Arduino interpreter on the Cyberdeck.

Built-in copies are embedded in application flash and work without a card or
mounted filesystem. Missing defaults and instructions are copied into existing
LittleFS `/badusb` and `/uart` folders without overwriting user files. Editable
copies take precedence. Explicit Initialize Storage formats only a completely
erased partition; non-erased/missing/damaged storage is preserved. Original
`/scripts` storage and the existing partition layout remain.

Macro parser limits: 8 KiB source, 256 expanded steps, 16 KiB expanded argument
budget, ten-minute execution limit. Unsupported commands fail validation before
any input is sent. Supported BadUSB subset includes STRING/STRINGLN, delays,
REPEAT, keyboard combinations, HOLD/RELEASE, ALTCHAR/ALTSTRING and mouse actions.
ID metadata does not change WOLF DECK USB descriptors. UART supports TX with
escapes, TXHEX, DELAY, WAIT_RX and REPEAT. Delay steps run cooperatively so touch,
web and UART capture continue. No scripts run on boot or on folder selection.

## Native USB

The ESP-IDF NAT build now compiles TinyUSB keyboard/mouse support. CDC support
is compiled too for Arduino 3.1.3 USB header compatibility; UART remains the
console and native USB is not started automatically. It starts
only on explicit ARM/Start HID. Connect ESP32-S3 native USB D- GPIO19 / D+
GPIO20 to the test host; a CH34x/CP210x USB-UART bridge cannot become HID.
Console remains UART; secondary USB Serial/JTAG console is disabled to avoid
claiming the native USB PHY. Existing USB pins remain reserved from GPIO tools.
US keyboard layout is used. Physical USB enumeration and stop behavior still
require device validation. Existing PlatformIO configuration remains gated;
this delivery uses the ESP-IDF NAT build.

## Configurable external module drivers

No external module pins are activated on boot. Saved wiring is loaded, then
Probe/Read/Transmit must be requested explicitly. GPIO assignments are checked
against LCD, touch, USB, console, flash/PSRAM and SD reservations and against
other configured modules. UART and ADC reject module-owned pins. Blank pin
list clears a saved assignment. All GPIO voltage levels are 3.3 V.

| ID | Pin order | Implemented software |
| --- | --- | --- |
| nfc | SDA,SCL,IRQ,RESET | PN532 firmware probe/SAM, ISO14443A UID reading |
| subghz | SCK,MISO,MOSI,CS | CC1101 version probe, frequency setup, RSSI, CRC-tagged packet RX, packet TX |
| ir | RX,TX | demodulated LOW pulse measurement, approx. 38 kHz NEC TX |
| ibutton | DATA | Dallas READ ROM and CRC8 check; external 4.7k pull-up |
| lf_rfid | RX,TX | 9600-baud RDM6300/EM4100 frame and checksum parsing |
| gpio | GPIO | manual input read and explicit output 0/1 |
| spi | SCK,MISO,MOSI,CS | mode 0 / 1 MHz full-duplex HEX transfer |
| i2c | SDA,SCL | external 100 kHz I2C bus scan |

Sub-GHz accepts 300–348, 387–464 and 779–928 MHz; the module/antenna must suit
the selected band. Default is 433.92 MHz. Packet baseline is 38.4 kbps 2-FSK,
variable length, CRC and whitening; peers must use matching radio settings.
This is not RAW OOK replay, arbitrary Flipper protocol decoding or rolling-code
emulation. SPI and CC1101 share the external SPI controller; external I2C and
PN532 share the second I2C controller. Probe the desired mode before use.
Onboard LCD SPI and touch I2C wiring/initialization remain unchanged.

## Explicit coverage limits

This is not complete Flipper Zero feature parity. NFC UID reading is not full
NFC memory access or card emulation. RDM6300 support does not emulate/write LF
RFID. Dallas READ ROM does not implement Cyfral/Metakom or iButton emulation.
IR currently covers NEC TX and pulse measurement, not all Flipper IR protocols.
U2F/FIDO authentication, all Bluetooth remote modes and arbitrary Flipper FAP
execution are not implemented. Flipper .sub/.ir/.nfc/.rfid/.ibtn text formats can
be stored and inspected; importing metadata does not imply protocol execution.
The `/apps/status` JSON reports `full_flipper_parity:false` and `u2f:false`.

Third-party driver sources are pinned under `components/adafruit_pn532` and
`components/arduino_tinyusb`, with upstream commits and licenses. Native parser,
format, CRC, pin-policy, scrollbar and flash-capture tests are part of the V6.2
Actions workflow. Actual module behavior, IR timing, radio RX/TX, USB and UART
peer communication require hardware tests. See delivery BUILD-RESULT.txt for
actual build/test status; a configured CI workflow is not evidence it ran.
