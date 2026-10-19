# WOLF DECK — V6.2 Full Access integration

Branch: `v6.2-full-access`, based on V6.1
`16e766162af5ad983ba4d37cf1021c7dec6bffe6`.

## Artwork

The three supplied JPEGs are retained in `assets/wolfdeck/source`.
`ci/wolfdeck_assets.py` generates checked-in flash data and its SHA-256 manifest.
The assets are embedded in the application image: no filesystem image,
SD card or extra upload is required.

- Web: supplied 1440 × 900 JPEG optimized to 1280 × 800, quality 78,
  progressive JPEG. 168,747 bytes, 41% smaller than the supplied web image.
  Served directly from flash, with browser caching. Shared CSS is injected
  into HTML responses, so existing manager pages receive the same theme.
  Dark overlays, opaque input/output surfaces, contrast, visible focus rings
  and mobile spacing preserve readability.
- Touch fit correction: the supplied 320 × 240 JPEGs were already close-up
  crops. Display and boot now use the complete composition from the supplied
  larger web image, scaled proportionally to 320 × 200 and centered at y=20
  on the 320 × 240 canvas, with 20-pixel dark bars above/below. No source edge
  is cropped and no stretching/zoom is used. The original files are retained.
  Runtime images are converted to 256-color indexed RGB565.
  Each image occupies 77,312 flash bytes including its palette. Only one
  640-byte scanline is used for drawing; there is no full-screen framebuffer,
  JPEG decoder or startup heap allocation for the pictures.
- Normal screens use the display artwork at half brightness. Home tiles
  show the same artwork at quarter brightness under their icons and labels.
  Existing coordinates, LCD initialization, orientation and touch conversion
  are unchanged.
- Boot: full-brightness fitted composition, typed WOLF DECK wordmark and V6.2 version,
  monotonic progress at actual display/touch/web/WiFi/storage/route milestones,
  then green READY. Missing touch/storage is reported in its stage text.
  READY means the UI has finished initialization, not that STA internet or
  every external module is available. Startup remains visible for at least
  five seconds, including one second of READY; slower initialization extends it.

Total picture data: 323,371 flash bytes; persistent graphics buffer: 640 bytes.
The application partition remains 6 MiB, within the unchanged 16 MiB layout.

## Functionality retained

The existing Full Access touch/web catalog, WiFi Setup bounds/history fix,
NAT code, WiFi profiles, BLE, UART, ADC, script manager and manual SD controls
are retained. Settings now reads current AP credentials from the WiFi manager,
including AP changes requested through web or the program catalog.
The Workbench revision adds configurable PN532 UID, CC1101 packet, IR NEC,
Dallas ROM, RDM6300, GPIO, SPI and external I2C drivers, plus native USB HID
and documented macro libraries. Exact protocol limits are in V62_WORKBENCH.md;
full Flipper Zero protocol coverage is not claimed.

## Navigation, credentials and keyboard

The touch home has Network, Bluetooth, Hardware, Files, System, Settings,
Radio and All Tools. The shared web catalog uses the same seven categories
plus All. Category tools are alphabetically sorted; native scanners, system
monitor, touch test and settings remain accessible alongside API programs.

The hotspot default is `wolfdeck`. A one-time migration applies this value
on first V6.2 startup, including existing installations. Subsequent custom
hotspot passwords are retained; upstream WiFi profiles and admin tokens are
not changed by this migration. Merged flashing can separately reset NVS.

The touch keyboard has its own 10 ms polling path and 35 ms bounce filter,
independent of the 220 ms menu debounce. It waits for release of the opening
touch, accepts repeated letters, highlights pressed letters and redraws only
the input preview while typing. Passwords use actual asterisk masking.
Mode, delete, space, DONE and multiline script content are supported.

## Integrated app icons

All 41 original WOLF DECK symbols are embedded. Touch uses the 24-pixel
PROGMEM mask set (2,952 bytes) on the home tiles, category lists, hardware/radio
tiles and page headers. Program actions/results use their app's icon. Existing
button rectangles, touch polling, WiFi Setup and keyboard logic are unchanged.
The shared mapping covers all 92 catalog routes and 29 native page IDs.
Web category buttons, original-page links and the selected program heading
show the matching SVG, served directly from flash at `/wolfdeck_icons/ID.svg`.
Icon routes are excluded from the program catalog. No external font/icon CDN,
filesystem image, extra RAM framebuffer or separate graphics flash is used.
`ci/wolfdeck_icon_firmware.py --check` verifies checked-in firmware tables;
it uses only Python standard-library modules. Raster design exports remain
outside the application image. The existing app version remains V6.2; use
SOURCE_COMMIT.txt to distinguish firmware revisions.

## Verification and limits

The V6.2-only Actions workflow includes 11 Python regressions, native shared
handler, keyboard, category/default-password and scanline/animation tests,
and mocked web UI tests, followed
by the ESP-IDF 5.3.2 NAT build and merged-image packaging. Tests check that
NAT code, LCD/touch wiring, coordinate conversion and partition table are
preserved. Native graphics tests compare every rendered palette pixel, test
invalid bounds, progress monotonicity, scanline size and bounded animation.

GitHub currently rejects branch creation with HTTP 403
`Resource not accessible by integration`; a successful local build does not
mean Actions ran. Actual LCD colors/refresh timing, touch, WiFi Setup stability,
BLE and Android-client NAT forwarding still require on-device verification.
See the delivery README for the actual build/test status, not merely this CI
configuration description. Stable V6.0/V6.1 branches and files are not changed.
