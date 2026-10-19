# WOLF DECK — bruksanvisning for BadUSB- og UART-skript

Velg Files -> BadUSB eller UART pa touch, eller /apps/scripts pa web.
Alle filer har HELP og CODE. Native USB starter manuelt med ARM; RUN krever
riktig test-PC. UART startes med RX/TX/baud i UART Lab. Ingen skript starter
automatisk. STOP avbryter makroen; STOP ALL stopper ogsa UART og moduler.

De fem .uart-makroene kjores pa ESP32-en. De fire .ino-filene er eksempler
for et separat Arduino-testkort. BadUSB bruker amerikansk tastaturoppsett;
pa Windows bor Num Lock vaere pa for Alt+numpad-demoen.

---

# demo_windows.txt

Hva: offisielt Flipper Zero-eksempel fra GitHub. Skriver Hello World og en teksttegning i Windows med Notisblokk.

Utstyr: ESP32-S3s native USB-port (GPIO19 D-/GPIO20 D+) koblet til din test-PC. USB-UART-port via CH34x/CP210x kan ikke fungere som HID. Velg amerikansk tastaturoppsett for nøyaktige symboler.

Fremgangsmåte:
1. Åpne Scripts -> BadUSB på touch eller web. Velg filen og les innholdet.
2. Kjør Validate først; ukjente kommandoer stopper før kjøring.
3. Klargjør et tomt testvindu og koble native USB til test-PC-en.
4. Start USB HID manuelt. Kjør den valgte filen og følg fremdriften.
5. STOP avbryter og slipper alle holdte taster/museknapper.

Forventet resultat: et teksteksempel; ingen filer lastes ned eller installeres.
ID-linjer beholdes som kildeinnhold; WOLF DECK bruker sin egen USB-identitet.
Ved feil tegn: kontroller PC-ens tastaturoppsett. Ved manglende HID: kontroller native USB-kobling og status. Ikke kjør en annen OS-demo enn den som passer test-PC-en.

Kilde: https://github.com/flipperdevices/flipperzero-firmware/blob/3fb1d51727733d2007abc5ec636a89328857803e/applications/main/bad_usb/resources/badusb/demo_windows.txt
Commit: 3fb1d51727733d2007abc5ec636a89328857803e
Lisens: GPL-3.0-only; full lisens følger i samme mappe.


---

# demo_macos.txt

Hva: offisielt Flipper Zero-eksempel fra GitHub. Skriver Hello World og en teksttegning i macOS med Terminal.

Utstyr: ESP32-S3s native USB-port (GPIO19 D-/GPIO20 D+) koblet til din test-PC. USB-UART-port via CH34x/CP210x kan ikke fungere som HID. Velg amerikansk tastaturoppsett for nøyaktige symboler.

Fremgangsmåte:
1. Åpne Scripts -> BadUSB på touch eller web. Velg filen og les innholdet.
2. Kjør Validate først; ukjente kommandoer stopper før kjøring.
3. Klargjør et tomt testvindu og koble native USB til test-PC-en.
4. Start USB HID manuelt. Kjør den valgte filen og følg fremdriften.
5. STOP avbryter og slipper alle holdte taster/museknapper.

Forventet resultat: et teksteksempel; ingen filer lastes ned eller installeres.
ID-linjer beholdes som kildeinnhold; WOLF DECK bruker sin egen USB-identitet.
Ved feil tegn: kontroller PC-ens tastaturoppsett. Ved manglende HID: kontroller native USB-kobling og status. Ikke kjør en annen OS-demo enn den som passer test-PC-en.

Kilde: https://github.com/flipperdevices/flipperzero-firmware/blob/3fb1d51727733d2007abc5ec636a89328857803e/applications/main/bad_usb/resources/badusb/demo_macos.txt
Commit: 3fb1d51727733d2007abc5ec636a89328857803e
Lisens: GPL-3.0-only; full lisens følger i samme mappe.


---

# demo_gnome.txt

Hva: offisielt Flipper Zero-eksempel fra GitHub. Skriver Hello World og en teksttegning i Linux/GNOME med terminalprogram.

Utstyr: ESP32-S3s native USB-port (GPIO19 D-/GPIO20 D+) koblet til din test-PC. USB-UART-port via CH34x/CP210x kan ikke fungere som HID. Velg amerikansk tastaturoppsett for nøyaktige symboler.

Fremgangsmåte:
1. Åpne Scripts -> BadUSB på touch eller web. Velg filen og les innholdet.
2. Kjør Validate først; ukjente kommandoer stopper før kjøring.
3. Klargjør et tomt testvindu og koble native USB til test-PC-en.
4. Start USB HID manuelt. Kjør den valgte filen og følg fremdriften.
5. STOP avbryter og slipper alle holdte taster/museknapper.

Forventet resultat: et teksteksempel; ingen filer lastes ned eller installeres.
ID-linjer beholdes som kildeinnhold; WOLF DECK bruker sin egen USB-identitet.
Ved feil tegn: kontroller PC-ens tastaturoppsett. Ved manglende HID: kontroller native USB-kobling og status. Ikke kjør en annen OS-demo enn den som passer test-PC-en.

Kilde: https://github.com/flipperdevices/flipperzero-firmware/blob/3fb1d51727733d2007abc5ec636a89328857803e/applications/main/bad_usb/resources/badusb/demo_gnome.txt
Commit: 3fb1d51727733d2007abc5ec636a89328857803e
Lisens: GPL-3.0-only; full lisens følger i samme mappe.


---

# demo_chromeos.txt

Hva: offisielt Flipper Zero-eksempel fra GitHub. Skriver Hello World og en teksttegning i Chrome/ChromeOS med nettleser.

Utstyr: ESP32-S3s native USB-port (GPIO19 D-/GPIO20 D+) koblet til din test-PC. USB-UART-port via CH34x/CP210x kan ikke fungere som HID. Velg amerikansk tastaturoppsett for nøyaktige symboler.

Fremgangsmåte:
1. Åpne Scripts -> BadUSB på touch eller web. Velg filen og les innholdet.
2. Kjør Validate først; ukjente kommandoer stopper før kjøring.
3. Klargjør et tomt testvindu og koble native USB til test-PC-en.
4. Start USB HID manuelt. Kjør den valgte filen og følg fremdriften.
5. STOP avbryter og slipper alle holdte taster/museknapper.

Forventet resultat: et teksteksempel; ingen filer lastes ned eller installeres.
ID-linjer beholdes som kildeinnhold; WOLF DECK bruker sin egen USB-identitet.
Ved feil tegn: kontroller PC-ens tastaturoppsett. Ved manglende HID: kontroller native USB-kobling og status. Ikke kjør en annen OS-demo enn den som passer test-PC-en.

Kilde: https://github.com/flipperdevices/flipperzero-firmware/blob/3fb1d51727733d2007abc5ec636a89328857803e/applications/main/bad_usb/resources/badusb/demo_chromeos.txt
Commit: 3fb1d51727733d2007abc5ec636a89328857803e
Lisens: GPL-3.0-only; full lisens følger i samme mappe.


---

# test_mouse.txt

Hva: offisielt Flipper Zero-eksempel fra GitHub. Tester museklikk, bevegelse, rulling og hold/slipp av taster.

Utstyr: ESP32-S3s native USB-port (GPIO19 D-/GPIO20 D+) koblet til din test-PC. USB-UART-port via CH34x/CP210x kan ikke fungere som HID. Velg amerikansk tastaturoppsett for nøyaktige symboler.

Fremgangsmåte:
1. Åpne Scripts -> BadUSB på touch eller web. Velg filen og les innholdet.
2. Kjør Validate først; ukjente kommandoer stopper før kjøring.
3. Klargjør et tomt testvindu og koble native USB til test-PC-en.
4. Start USB HID manuelt. Kjør den valgte filen og følg fremdriften.
5. STOP avbryter og slipper alle holdte taster/museknapper.

Forventet resultat: pekeren beveger seg i en firkant og klikk/rulling testes.
ID-linjer beholdes som kildeinnhold; WOLF DECK bruker sin egen USB-identitet.
Ved feil tegn: kontroller PC-ens tastaturoppsett. Ved manglende HID: kontroller native USB-kobling og status. Ikke kjør en annen OS-demo enn den som passer test-PC-en.

Kilde: https://github.com/flipperdevices/flipperzero-firmware/blob/3fb1d51727733d2007abc5ec636a89328857803e/applications/main/bad_usb/resources/badusb/test_mouse.txt
Commit: 3fb1d51727733d2007abc5ec636a89328857803e
Lisens: GPL-3.0-only; full lisens følger i samme mappe.


---

# ASCIITable.ino

Dette er en original Arduino-skisse fra GitHub, ikke en makro som kan kjøres på Cyberdecken. Åpne skissen i Arduino IDE og flash den på et separat testkort. Bruk den tilsvarende nummererte .uart-makroen på Cyberdecken. Kommunikasjonseksemplene bruker 9600 baud. Koble felles jord og TX/RX krysset; ESP32 tåler bare 3,3 V på inngangene. Følg koblingen forklart i skissens kildekommentarer.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/blob/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication/ASCIITable/ASCIITable.ino
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: Public domain (stated in source)


---

# PhysicalPixel.ino

Dette er en original Arduino-skisse fra GitHub, ikke en makro som kan kjøres på Cyberdecken. Åpne skissen i Arduino IDE og flash den på et separat testkort. Bruk den tilsvarende nummererte .uart-makroen på Cyberdecken. Kommunikasjonseksemplene bruker 9600 baud. Koble felles jord og TX/RX krysset; ESP32 tåler bare 3,3 V på inngangene. Følg koblingen forklart i skissens kildekommentarer.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/blob/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication/PhysicalPixel/PhysicalPixel.ino
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: Public domain (stated in source)


---

# SerialCallResponse.ino

Dette er en original Arduino-skisse fra GitHub, ikke en makro som kan kjøres på Cyberdecken. Åpne skissen i Arduino IDE og flash den på et separat testkort. Bruk den tilsvarende nummererte .uart-makroen på Cyberdecken. Kommunikasjonseksemplene bruker 9600 baud. Koble felles jord og TX/RX krysset; ESP32 tåler bare 3,3 V på inngangene. Følg koblingen forklart i skissens kildekommentarer.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/blob/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication/SerialCallResponse/SerialCallResponse.ino
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: Public domain (stated in source)


---

# SerialEvent.ino

Dette er en original Arduino-skisse fra GitHub, ikke en makro som kan kjøres på Cyberdecken. Åpne skissen i Arduino IDE og flash den på et separat testkort. Bruk den tilsvarende nummererte .uart-makroen på Cyberdecken. Kommunikasjonseksemplene bruker 9600 baud. Koble felles jord og TX/RX krysset; ESP32 tåler bare 3,3 V på inngangene. Følg koblingen forklart i skissens kildekommentarer.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/blob/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication/SerialEvent/SerialEvent.ino
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: Public domain (stated in source)


---

# 01_loopback.uart

Koble TX til RX på Cyberdecken for loopback. Start UART med to ledige GPIO-pinner og 9600 baud. Kjør makroen; capture_hex skal vise teksten som ble sendt. Koble fra løkken når testen er ferdig.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain


---

# 02_physical_pixel.uart

Flash PhysicalPixel.ino på et separat Arduino-kort. Forbind jord, Cyberdeck TX til mottakerens RX via riktige spenningsnivåer. Start UART 9600 baud. H tenner testkortets LED og L slukker den. Unngå 5 V på ESP32-innganger.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain


---

# 03_call_response.uart

Flash SerialCallResponse.ino på testkortet. Start UART med 9600 baud. Makroen sender to forespørsler; mottakeren svarer med tre målebytes per forespørsel. Capture vises som HEX. Testkortets analoge innganger må kobles etter Arduino-eksemplet.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain


---

# 04_serial_event.uart

Flash SerialEvent.ino på testkortet, start UART 9600 baud og kjør. Mottakeren samler tegn frem til linjeskift og skriver linjen tilbake. Kontroller teksten i capture.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain


---

# 05_ascii_table_receive.uart

Flash ASCIITable.ino på testkortet. Start UART 9600 baud, kjør makroen og trykk reset på testkortet. Appen venter fem sekunder mens UART-mottak fortsetter. Åpne capture_hex for ASCII-tabellen. Krever ingen sending fra Cyberdecken.

Start UART manuelt med riktig RX, TX og baud før Run. Makroene kjøres trinnvis uten å blokkere UI. STOP avbryter neste trinn; UART kan stoppes separat.

Kilde: https://github.com/arduino/arduino-examples/tree/50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a/examples/04.Communication
Commit: 50b66a3ce1e4e5e69cd96d3af5ca41a3383b311a
Lisens: CC0-1.0 (original WOLF DECK macro); Arduino reference public domain


