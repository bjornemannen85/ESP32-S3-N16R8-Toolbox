#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "v61_wifi.h"
#include "v61_scripts.h"

// V6.1 61-80: authenticated diagnostics and safe workflow utilities.
// Does not activate USB HID, BLE GATT, NAT, or unverified GPIO.
namespace V61Bundle6180 {
struct ScanRecord { String ssid,bssid; int32_t rssi,channel; unsigned long when; };
static ScanRecord history[32];
static size_t historyCount=0,historyNext=0;
static String quoted(const String &s){return String("\"")+V61WiFi::escapeJson(s)+"\"";}
static void record(const String &ssid,const String &bssid,int32_t rssi,int32_t channel){
 history[historyNext]={ssid,bssid,rssi,channel,millis()/1000};
 historyNext=(historyNext+1)%32;if(historyCount<32)historyCount++;
}
static String scanJson(){
 int count=WiFi.scanNetworks(false,true);
 if(count<0)return String("{\"ok\":false,\"error\":\"scan failed\"}");
 String j="{\"ok\":true,\"count\":"+String(count)+",\"networks\":[";
 int limit=count<32?count:32;
 for(int i=0;i<limit;i++){
  if(i)j+=",";
  String ssid=WiFi.SSID(i),bssid=WiFi.BSSIDstr(i);
  int32_t rssi=WiFi.RSSI(i),channel=WiFi.channel(i);
  record(ssid,bssid,rssi,channel);
  j+="{\"ssid\":"+quoted(ssid)+",\"bssid\":"+quoted(bssid)+",\"rssi\":"+String(rssi)+",\"channel\":"+String(channel)+"}";
 }
 WiFi.scanDelete();return j+"]}";
}
static String historyJson(){
 String j="{\"count\":"+String(historyCount)+",\"entries\":[";
 for(size_t i=0;i<historyCount;i++){
  size_t idx=(historyNext+32-historyCount+i)%32;const ScanRecord &r=history[idx];
  if(i)j+=",";
  j+="{\"ssid\":"+quoted(r.ssid)+",\"bssid\":"+quoted(r.bssid)+",\"rssi\":"+String(r.rssi)+",\"channel\":"+String(r.channel)+",\"uptime_seconds\":"+String(r.when)+"}";
 }
 return j+"]}";
}
static String csvSafe(String s){
 s.replace("\"","\"\"");
 if(s.startsWith("=")||s.startsWith("+")||s.startsWith("-")||s.startsWith("@"))s="'"+s;
 return String("\"")+s+"\"";
}
static void routes(WebServer &s){
 s.on("/api/v61/scan/live",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",scanJson());});
 s.on("/api/v61/scan/history",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",historyJson());});
 s.on("/api/v61/scan/history.csv",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String out="ssid,bssid,rssi,channel,uptime_seconds\n";
  for(size_t i=0;i<historyCount;i++){size_t idx=(historyNext+32-historyCount+i)%32;const ScanRecord &r=history[idx];out+=csvSafe(r.ssid)+","+csvSafe(r.bssid)+","+String(r.rssi)+","+String(r.channel)+","+String(r.when)+"\n";}
  s.sendHeader("Content-Disposition","attachment; filename=wifi-history.csv");s.send(200,"text/csv; charset=utf-8",out);
 });
 s.on("/api/v61/scan/clear",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);historyCount=historyNext=0;s.send(200,"application/json","{\"ok\":true}");});
 s.on("/api/v61/scripts/summary",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  if(!V61Scripts::ready){s.send(503,"application/json","{\"ready\":false}");return;}
  File dir=LittleFS.open("/scripts");if(!dir||!dir.isDirectory()){s.send(503,"application/json","{\"ready\":false}");return;}
  size_t count=0,bytes=0;File f=dir.openNextFile();while(f){if(!f.isDirectory()){count++;bytes+=f.size();}f.close();f=dir.openNextFile();}dir.close();
  s.send(200,"application/json",String("{\"ready\":true,\"files\":")+String(count)+",\"bytes\":"+String(bytes)+",\"total_bytes\":"+String(LittleFS.totalBytes())+",\"used_bytes\":"+String(LittleFS.usedBytes())+"}");
 });
 s.on("/api/v61/uart/safety",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  s.send(200,"application/json","{\"uart_active\":false,\"gpio_write_active\":false,\"reserved_pins\":[10,11,12,13,15,16,18,19,20,45,46],\"note\":\"Read-only guidance; pinout verification required before UART or GPIO activation\"}");
 });
 s.on("/api/v61/roadmap",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  s.send(200,"application/json","{\"wifi_history\":true,\"wifi_csv\":true,\"script_summary\":true,\"usb_hid\":false,\"ble_gatt_transfer\":false,\"nat\":false,\"uart_io\":false,\"gpio_write\":false,\"hardware_tested\":false}");
 });
 s.on("/toolbox/operations",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck Operations</title><style>body{background:#081525;color:#dff;font:16px sans-serif;max-width:760px;margin:auto;padding:16px}button,input{background:#142d43;color:#fff;border:1px solid #0cf;padding:10px;margin:4px;border-radius:6px}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style></head><body><h2>Cyberdeck V6.1 Operations</h2><p>Authenticated diagnostics. Scans are manual. History is RAM-only and limited to 32 entries.</p><input id="token" type="password" placeholder="Admin token"><div><button onclick="req('/api/v61/scan/live')">Scan Wi-Fi</button><button onclick="req('/api/v61/scan/history')">History</button><button onclick="downloadCsv()">CSV</button><button onclick="clearHistory()">Clear history</button><button onclick="req('/api/v61/scripts/summary')">Scripts</button><button onclick="req('/api/v61/uart/safety')">Pin safety</button><button onclick="req('/api/v61/roadmap')">Features</button></div><pre id="output"></pre><a href="/toolbox/pro">Toolbox Pro</a><script>const el=id=>document.getElementById(id);async function req(path,method='GET'){let r=await fetch(path,{method,headers:{'X-Cyberdeck-Token':el('token').value}});el('output').textContent=(r.ok?'':'HTTP '+r.status+' ')+await r.text()}function clearHistory(){if(confirm('Clear Wi-Fi history?'))req('/api/v61/scan/clear','POST')}async function downloadCsv(){let r=await fetch('/api/v61/scan/history.csv',{headers:{'X-Cyberdeck-Token':el('token').value}});if(!r.ok){el('output').textContent='HTTP '+r.status;return}let blob=await r.blob(),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='wifi-history.csv';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)}</script></body></html>)HTML");});
}
}
