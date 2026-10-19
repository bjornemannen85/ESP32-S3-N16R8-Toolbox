# 05_ascii_table_receive.uart

Flash ASCIITable.ino på testkortet. Start UART 9600 baud, kjør makroen og trykk reset på testkortet. Appen venter fem sekunder mens UART-mottak fortsetter. Åpne capture_hex for ASCII-tabellen. Krever ingen sending fra Cyberdecken.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain
