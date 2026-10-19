# 04_serial_event.uart

Flash SerialEvent.ino på testkortet, start UART 9600 baud og kjør. Mottakeren samler tegn frem til linjeskift og skriver linjen tilbake. Kontroller teksten i capture.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain
