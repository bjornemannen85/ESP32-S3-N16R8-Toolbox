# 01_loopback.uart

Koble TX til RX på Cyberdecken for loopback. Start UART med to ledige GPIO-pinner og 9600 baud. Kjør makroen; capture_hex skal vise teksten som ble sendt. Koble fra løkken når testen er ferdig.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain
