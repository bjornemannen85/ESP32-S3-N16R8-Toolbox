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
- Touch: both 320 × 240 originals converted to 256-color indexed RGB565.
  Each image occupies 77,312 flash bytes including its palette. Only one
  640-byte scanline is used for drawing; there is no full-screen framebuffer,
  JPEG decoder or startup heap allocation for the pictures.
- Normal screens use the display artwork at half brightness. Home tiles
  show the same artwork at quarter brightness under their icons and labels.
  Existing coordinates, LCD initialization, orientation and touch conversion
  are unchanged.
- Boot: full-brightness boot artwork, six-frame WOLF DECK wordmark fade,
  monotonic progress at actual display/touch/web/WiFi/storage/route milestones,
  then green READY. Missing touch/storage is reported in its stage text.
  READY means the UI has finished initialization, not that STA internet or
  every external module is available. Animation adds 670 ms in total;
  driver/network initialization takes its normal additional time.

Total picture data: 323,371 flash bytes; persistent graphics buffer: 640 bytes.
The application partition remains 6 MiB, within the unchanged 16 MiB layout.

## Functionality retained

The existing Full Access touch/web catalog, WiFi Setup bounds/history fix,
NAT code, WiFi profiles, BLE, UART, ADC, script manager and manual SD controls
are retained. Settings now reads current AP credentials from the WiFi manager,
including AP changes requested through web or the program catalog.
Unimplemented NFC/CC1101/IR drivers and the existing USB HID gate remain
accurately identified rather than reported as working.

## Verification and limits

The V6.2-only Actions workflow includes 10 Python regressions, native shared
handler tests, native scanline/animation tests and mocked web UI tests, followed
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
