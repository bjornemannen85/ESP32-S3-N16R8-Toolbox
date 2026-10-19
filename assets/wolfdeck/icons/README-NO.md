# WOLF DECK — appikoner V1

41 originale symboler i samme geometriske stil som Cyberdeck-menyene.
Fargene skiller Network, Bluetooth, Hardware, Files, System, Settings og Radio.
Ikonene har transparent bakgrunn og ingen innbakt tekst, slik at etiketter kan
oversettes og forbli lesbare på 320 × 240-skjermen.

## Innhold

- `svg/`: skalerbare weboriginaler, 64 × 64 koordinater og 3-enheters strek.
- `png-16/`, `png-24/`, `png-32/`, `png-48/`, `png-64/`, `png-128/`:
  transparente PNG-filer. Bruk 24/32 piksler på skjermen, 32/48 på web.
- `wolfdeck_icons_24.h` og `wolfdeck_icons_32.h`: ferdige 1-bits maskebilder
  i PROGMEM. Hele settet bruker henholdsvis 2 952 og 5 248 flash-byte.
  Inkluder bare én størrelse i firmware; farge velges ved tegning.
- `manifest.json`: alle 92 eksisterende katalogruter, alle 29 skjermsider,
  kategorier og hjemknapper koblet til ikon-ID. Flere handlinger innen samme
  app deler ikonet, for eksempel UART start/stopp/status.
- `WOLF-DECK-App-Icons.png`: samlet oversikt, med 24/32-pikslers eksempler.
- `SHA256SUMS.txt`: kontrollsummer.

Eksempel med eksisterende Adafruit_GFX-driver:

```cpp
#include "wolfdeck_icons_24.h"
lcd.drawBitmap(x, y, WolfDeckIcons24::uart, 24, 24, 0xFE2A);
```

Tegning uten bakgrunnsfarge bevarer bakgrunnen under maskens nullbiter.
32-pikslers variant bruker `WolfDeckIcons32::uart` og størrelse 32 × 32.
Navn med bindestrek blir understrek i C++, eksempel `ble_transfer`.
Manifestets HEX-farger konverteres til RGB565 ved behov. Ikonfilene inneholder
ikke kode som endrer touch, NAT, WiFi eller øvrige programfunksjoner.

Webeksempel:

```html
<img src="svg/uart.svg" width="32" height="32" alt="UART terminal">
```

Dette er en grafikkpakke klargjort for integrering. Den endrer ikke den
allerede leverte BIN-filen og er ikke i seg selv flashbar firmware.
Kildegeneratoren er `ci/wolfdeck_icons.py` på grenen `v6.2-full-access`.
Generer med Python, Pillow 12.3.0 og CairoSVG 2.9.1.
V6.0 og fungerende V6.1-filer er ikke endret.
