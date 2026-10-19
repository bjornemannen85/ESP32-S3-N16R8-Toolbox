# 03_call_response.uart

Flash SerialCallResponse.ino på testkortet. Start UART med 9600 baud. Makroen sender to forespørsler; mottakeren svarer med tre målebytes per forespørsel. Capture vises som HEX. Testkortets analoge innganger må kobles etter Arduino-eksemplet.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain
