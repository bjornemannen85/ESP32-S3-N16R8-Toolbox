# V6.1 Wi-Fi development (NOT compiled or hardware-tested)

- NVS profiles (up to eight), priority and rotating reconnection attempts.
- AP SSID/password persisted, touch keyboard and web management.
- Wi-Fi scan no longer explicitly disconnects STA uplink.
- SDMMC boot mount temporarily disabled until GPIO mapping is verified.
- Web DNS and HTTP HEAD diagnostics available.
- **Not yet implemented:** actual NAT forwarding, ping, verified secure credential transport, robust connection selection based on scan results.
- **Important:** existing HTTP admin-token transport is unencrypted; do not expose AP to untrusted clients.
- No firmware flashing or hardware tests performed.

## Stage 3 (source patch; not yet compiled or tested)
- Optional lwIP NAPT enable/disable on AP address, only when compiled with `lwip/lwip_napt.h`.
- Authenticated `POST /wifi/nat` with `enabled=1` or `0`; persisted in NVS.
- Authenticated `GET /wifi/tcp?host=example.com&port=80` TCP connection latency probe (not ICMP ping).
- `/wifi/status` reports NAT support/requested/attempted activation; real forwarding must be physically tested.
- Router DNS/DHCP advertisement, upstream reachability and actual client forwarding are not yet verified.
