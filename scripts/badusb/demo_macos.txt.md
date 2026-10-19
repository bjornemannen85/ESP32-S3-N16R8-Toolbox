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
