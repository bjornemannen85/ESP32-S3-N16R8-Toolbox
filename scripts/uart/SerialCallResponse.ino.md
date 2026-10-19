# SerialCallResponse.ino

Dette er en original Arduino-skisse fra GitHub, ikke en makro som kan kjøres på Cyberdecken. Åpne skissen i Arduino IDE og flash den på et separat testkort. Bruk den tilsvarende nummererte .uart-makroen på Cyberdecken. Kommunikasjonseksemplene bruker 9600 baud. Koble felles jord og TX/RX krysset; ESP32 tåler bare 3,3 V på inngangene. Følg koblingen forklart i skissens kildekommentarer.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/blob/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication/SerialCallResponse/SerialCallResponse.ino
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: Public domain (stated in source)
