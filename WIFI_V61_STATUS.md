# V6.1 Wi-Fi Manager (development, NOT hardware-tested)
- NVS persistence: up to 8 known SSIDs, priorities and credentials
- AP SSID/password change and persistence, authenticated web admin token
- STA reconnect, AP+STA, scan, DNS and HTTP HEAD diagnostics
- Web dashboard: /wifi, local token shown on physical Settings page
- Security: token in NVS, no credentials in status JSON; HTTP admin token is not encrypted over local AP. Use only on trusted devices.
- Not implemented: IPv4 NAT forwarding, on-screen touch keyboard, ping, roaming among multiple available networks, OTA.
- Do not flash until SDMMC pin map is verified and hardware validation planned.
