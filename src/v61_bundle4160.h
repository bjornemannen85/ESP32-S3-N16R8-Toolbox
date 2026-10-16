#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "v61_wifi.h"
#include "v61_scripts.h"
// V6.1 41-60: bounded, authenticated, non-invasive diagnostics.
namespace V61Bundle4160 {
struct Snapshot { unsigned long at; int count; int strongest; };
static Snapshot history[12];
static uint8_t head=0, used=0;
static String q(const String &s){return String("\"")+V61WiFi::escapeJson(s)+"\"";}
static bool auth(WebServer &s){if(V61WiFi::authorized(s))return true;V61WiFi::forbidden(s);return false;}
static void routes(WebServer &s){
 s.on("/api/v61/v2/features",HTTP_GET,[&s](){if(!auth(s))return;
  s.send(200,"application/json","{\"bundle\":\"41-60\",\"wifi_history\":true,\"script_search\":true,\"ble_scan\":\"existing_touch_ui_only\",\"ble_file_transfer\":false,\"usb_hid\":false,\"nat\":false,\"uart_active\":false,\"gpio_active\":false,\"spi_active\":false}");});
 s.on("/api/v61/v2/wifi/snapshot",HTTP_GET,[&s](){if(!auth(s))return;
  // Explicitly triggered; do not scan in background or alter STA profiles.
  int n=WiFi.scanNetworks(false,true);if(n<0){s.send(503,"application/json","{\"error\":\"scan unavailable\"}");return;}
  int best=-127;String j="{\"networks\":[";int limit=n<30?n:30;
  for(int i=0;i<limit;i++){if(i)j+=",";int r=WiFi.RSSI(i);if(r>best)best=r;
   j+="{\"ssid\":"+q(WiFi.SSID(i))+",\"bssid\":"+q(WiFi.BSSIDstr(i))+",\"rssi\":"+String(r)+",\"channel\":"+String(WiFi.channel(i))+"}";}
  history[head]={millis(),n,best};head=(head+1)%12;if(used<12)used++;
  j+="],\"total\":"+String(n)+",\"returned\":"+String(limit)+",\"history_samples\":"+String(used)+"}";
  WiFi.scanDelete();s.send(200,"application/json",j);
 });
 s.on("/api/v61/v2/wifi/history",HTTP_GET,[&s](){if(!auth(s))return;
  String j="{\"history\":[";for(int i=0;i<used;i++){int ix=(head+12-used+i)%12;if(i)j+=",";
   j+="{\"uptime_ms\":"+String(history[ix].at)+",\"count\":"+String(history[ix].count)+",\"strongest_rssi\":"+String(history[ix].strongest)+"}";}
  s.send(200,"application/json",j+"]}");});
 s.on("/api/v61/v2/wifi/history/clear",HTTP_POST,[&s](){if(!auth(s))return;head=used=0;s.send(200,"application/json","{\"ok\":true}");});
 s.on("/api/v61/v2/scripts/search",HTTP_GET,[&s](){if(!auth(s))return;
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  String term=s.arg("q");term.toLowerCase();if(term.length()>48){s.send(400,"text/plain","Search too long");return;}
  File d=LittleFS.open("/scripts");if(!d||!d.isDirectory()){s.send(500,"text/plain","Directory unavailable");return;}
  String j="{\"matches\":[";int found=0;File f=d.openNextFile();
  while(f){String n=String(f.name());int slash=n.lastIndexOf('/');if(slash>=0)n=n.substring(slash+1);
   String lower=n;lower.toLowerCase();if(!f.isDirectory()&&V61Scripts::validName(n)&&lower.indexOf(term)>=0){if(found)j+=",";j+="{\"name\":"+q(n)+",\"bytes\":"+String(f.size())+"}";found++;}
   f.close();if(found>=50)break;f=d.openNextFile();}
  d.close();s.send(200,"application/json",j+"],\"count\":"+String(found)+"}");});
 s.on("/api/v61/v2/scripts/usage",HTTP_GET,[&s](){if(!auth(s))return;
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  File d=LittleFS.open("/scripts");if(!d||!d.isDirectory()){s.send(500,"text/plain","Directory unavailable");return;}
  int count=0;size_t bytes=0;File f=d.openNextFile();while(f){if(!f.isDirectory()){count++;bytes+=f.size();}f.close();f=d.openNextFile();}d.close();
  s.send(200,"application/json","{\"files\":"+String(count)+",\"file_bytes\":"+String(bytes)+",\"used_bytes\":"+String(LittleFS.usedBytes())+",\"total_bytes\":"+String(LittleFS.totalBytes())+"}");});
 s.on("/api/v61/v2/hardware/plan",HTTP_GET,[&s](){if(!auth(s))return;
  s.send(200,"application/json","{\"reserved\":[10,46,12,11,13,45,16,15,18,19,20],\"i2c_touch\":{\"sda\":16,\"scl\":15},\"uart\":\"disabled_pending_pin_validation\",\"gpio_writes\":false,\"spi_transactions\":false,\"sd_mmc\":\"disabled_pending_pin_validation\"}");});
 s.on("/api/v61/v2/runtime",HTTP_GET,[&s](){if(!auth(s))return;
  s.send(200,"application/json","{\"uptime_ms\":"+String(millis())+",\"free_heap\":"+String(ESP.getFreeHeap())+",\"min_heap\":"+String(ESP.getMinFreeHeap())+",\"free_psram\":"+String(ESP.getFreePsram())+",\"ap_clients\":"+String(WiFi.softAPgetStationNum())+"}");});
 s.on("/toolbox/advanced",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck Advanced</title><style>body{background:#071526;color:#def;font:16px sans-serif;max-width:780px;margin:auto;padding:15px}button,input{background:#18324a;color:white;border:1px solid #0cf;padding:10px;margin:5px}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h2>Cyberdeck V6.1 Advanced</h2><p>Read-only diagnostics. Wi-Fi scans are manual. No HID, GPIO output or BLE file transfer.</p><input id="token" type="password" placeholder="Admin token"><button onclick="req('/api/v61/v2/features')">Features</button><button onclick="req('/api/v61/v2/wifi/snapshot')">Scan Wi-Fi</button><button onclick="req('/api/v61/v2/wifi/history')">History</button><button onclick="req('/api/v61/v2/scripts/usage')">Storage</button><button onclick="req('/api/v61/v2/runtime')">Runtime</button><button onclick="req('/api/v61/v2/hardware/plan')">Pins</button><input id="query" placeholder="Search script filenames"><button onclick="req('/api/v61/v2/scripts/search?q='+encodeURIComponent(document.getElementById('query').value))">Search</button><pre id="out"></pre><a href="/toolbox/pro">Pro dashboard</a><script>async function req(p){let r=await fetch(p,{headers:{'X-Cyberdeck-Token':document.getElementById('token').value}});document.getElementById('out').textContent='HTTP '+r.status+'\n'+await r.text()}</script>)HTML");});
}
}
