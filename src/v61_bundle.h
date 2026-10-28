#pragma once
String v62LiveFeatures();
#include <Arduino.h>
#include <WiFi.h>
#include "v62_server.h"
#include <LittleFS.h>
#include <Wire.h>
#include "v61_wifi.h"
#include "v61_scripts.h"
namespace V61Bundle {
static String quote(const String &v){return String("\"")+V61WiFi::escapeJson(v)+"\"";}
static void routes(V62Server &s){
 // 16: consolidated read-only dashboard, with links to existing tools.
 s.on("/toolbox",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"PAGE(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>Cyberdeck Tools</title><style>body{background:#071729;color:#dff;font:16px sans-serif;max-width:700px;margin:20px auto;padding:12px}a{color:#4df;display:block;padding:9px;border:1px solid #257;margin:8px 0;border-radius:6px}</style><h2>Cyberdeck V6.1 Toolbox</h2><p>Read-only diagnostics are available. Use the existing Wi-Fi and Script Manager pages for device management.</p><a href="/wifi">Wi-Fi Manager</a><a href="/scripts">Script Manager</a><a href="/toolbox/help">API reference</a><a href="/">Home</a>)PAGE");});
 // 17: API documentation.
 s.on("/toolbox/help",HTTP_GET,[&s](){s.send(200,"text/plain; charset=utf-8","GET /toolbox/health, /toolbox/wifi, /toolbox/memory, /toolbox/flash, /toolbox/storage, /toolbox/i2c, /toolbox/pins, /toolbox/features, /toolbox/clock\nNo admin token required. Diagnostics only; no pin writes, script execution, NAT, or SD mounting.");});
 // 18: uptime and CPU.
 s.on("/toolbox/health",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",String("{\"uptime_ms\":")+String(millis())+",\"cpu_mhz\":"+String(ESP.getCpuFreqMHz())+",\"free_heap\":"+String(ESP.getFreeHeap())+"}");});
 // 19: STA/AP link diagnostics.
 s.on("/toolbox/wifi",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);bool up=WiFi.status()==WL_CONNECTED;String j=String("{\"connected\":")+(up?"true":"false")+",\"ssid\":"+quote(up?WiFi.SSID():String("")) + ",\"rssi\":"+String(up?WiFi.RSSI():0)+",\"channel\":"+String(up?WiFi.channel():0)+",\"ap_clients\":"+String(WiFi.softAPgetStationNum())+"}";s.send(200,"application/json",j);});
 // 20: heap/PSRAM.
 s.on("/toolbox/memory",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",String("{\"free_heap\":")+String(ESP.getFreeHeap())+",\"heap_size\":"+String(ESP.getHeapSize())+",\"psram_size\":"+String(ESP.getPsramSize())+",\"free_psram\":"+String(ESP.getFreePsram())+"}");});
 // 21: flash geometry, not a filesystem capacity promise.
 s.on("/toolbox/flash",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",String("{\"flash_bytes\":")+String(ESP.getFlashChipSize())+",\"sketch_bytes\":"+String(ESP.getSketchSize())+",\"free_sketch_bytes\":"+String(ESP.getFreeSketchSpace())+"}");});
 // 22: LittleFS read-only stats.
 s.on("/toolbox/storage",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);if(!V61Scripts::ready){s.send(503,"application/json","{\"ready\":false}");return;}s.send(200,"application/json",String("{\"ready\":true,\"total\":")+String(LittleFS.totalBytes())+",\"used\":"+String(LittleFS.usedBytes())+"}");});
 // 23: I2C scan only on already initialized Wire pins; never writes to devices.
 s.on("/toolbox/i2c",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String j="{\"addresses\":[";bool first=true;for(uint8_t a=8;a<120;a++){Wire.beginTransmission(a);if(Wire.endTransmission()==0){if(!first)j+=",";first=false;j+=String(a);}}s.send(200,"application/json",j+"]}");});
 // 24: pin policy and unverified external peripherals.
 s.on("/toolbox/pins",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json","{\"display\":\"ILI9341\",\"touch\":\"FT6336G\",\"touch_sda\":16,\"touch_scl\":15,\"reserved_usb\":[19,20],\"external_module_pins_verified\":false,\"gpio_writes_enabled\":false}");});
 // 25: explicit truthful feature gates.
 s.on("/toolbox/features",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",v62LiveFeatures());});
 s.on("/toolbox/clock",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",String("{\"uptime_seconds\":")+String(millis()/1000)+",\"wall_clock_synchronized\":false}");});
}
}
