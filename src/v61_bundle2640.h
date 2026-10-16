#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Wire.h>
#include "v61_wifi.h"
#include "v61_scripts.h"

// Read-only diagnostics and authenticated script import. No HID, NAT, SD or RF writes.
namespace V61Bundle2640 {
static String jsonString(const String &s){return String("\"")+V61WiFi::escapeJson(s)+"\"";}
static void routes(WebServer &s){
 s.on("/api/v61/features",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  s.send(200,"application/json","{\"bundle\":\"26-40\",\"wifi\":true,\"scripts\":true,\"ble_transfer\":false,\"usb_hid\":false,\"nat\":false,\"sd\":false,\"rf\":false,\"nfc\":false,\"ir\":false}");});
 s.on("/api/v61/network",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  bool connected=WiFi.status()==WL_CONNECTED;
  String j="{\"connected\":"+String(connected?"true":"false")+",\"ssid\":"+jsonString(connected?WiFi.SSID():String(""))+
    ",\"rssi\":"+String(connected?WiFi.RSSI():0)+",\"channel\":"+String(connected?WiFi.channel():0)+
    ",\"ip\":"+jsonString(connected?WiFi.localIP().toString():String(""))+
    ",\"gateway\":"+jsonString(connected?WiFi.gatewayIP().toString():String(""))+
    ",\"dns\":"+jsonString(connected?WiFi.dnsIP().toString():String(""))+
    ",\"ap_clients\":"+String(WiFi.softAPgetStationNum())+"}";
  s.send(200,"application/json",j);});
 s.on("/api/v61/health",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String j="{\"uptime_ms\":"+String(millis())+",\"free_heap\":"+String(ESP.getFreeHeap())+
   ",\"min_free_heap\":"+String(ESP.getMinFreeHeap())+",\"free_psram\":"+String(ESP.getFreePsram())+
   ",\"flash_bytes\":"+String(ESP.getFlashChipSize())+",\"littlefs_ready\":"+String(V61Scripts::ready?"true":"false")+"}";
  s.send(200,"application/json",j);});
 s.on("/api/v61/scripts/import",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String name=s.arg("name"),body=s.arg("content");
  if(!V61Scripts::validName(name)){s.send(400,"text/plain","Invalid filename");return;}
  if(body.length()>V61Scripts::MAX_SCRIPT){s.send(413,"text/plain","File exceeds 8 KB");return;}
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  String path=V61Scripts::path(name);
  if(LittleFS.exists(path)){s.send(409,"text/plain","File exists; import does not overwrite");return;}
  File f=LittleFS.open(path,"w");if(!f){s.send(507,"text/plain","Unable to create file");return;}
  size_t written=f.write((const uint8_t*)body.c_str(),body.length());f.close();
  if(written!=body.length()){LittleFS.remove(path);s.send(507,"text/plain","Incomplete write");return;}
  s.send(201,"application/json","{\"ok\":true}");});
 s.on("/api/v61/scripts/catalog",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  File dir=LittleFS.open("/scripts");if(!dir||!dir.isDirectory()){s.send(500,"text/plain","Directory unavailable");return;}
  String j="{\"files\":[";int count=0;size_t bytes=0;File f=dir.openNextFile();
  while(f){String name=String(f.name());int slash=name.lastIndexOf('/');if(slash>=0)name=name.substring(slash+1);
   if(!f.isDirectory()&&V61Scripts::validName(name)){if(count)j+=",";j+="{\"name\":"+jsonString(name)+",\"bytes\":"+String(f.size())+"}";count++;bytes+=f.size();}
   f.close();f=dir.openNextFile();if(count>=100)break;
  }
  dir.close();j+=String("],\"count\":")+count+",\"total_bytes\":"+String(bytes)+"}";s.send(200,"application/json",j);});
 s.on("/toolbox/pro",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"V61HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck Pro</title><style>body{background:#071627;color:#dff;font:16px sans-serif;max-width:760px;margin:auto;padding:16px}input,textarea,button{background:#142a42;color:white;border:1px solid #0cf;border-radius:6px;padding:10px;margin:5px 0}input,textarea{box-sizing:border-box;width:100%}textarea{height:160px}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h2>Cyberdeck V6.1 Pro Tools</h2><p>Read-only diagnostics and manual script import. No automatic execution.</p><input id="token" type="password" placeholder="Admin token"><button onclick="get('/api/v61/features')">Features</button><button onclick="get('/api/v61/network')">Wi-Fi</button><button onclick="get('/api/v61/health')">Health</button><button onclick="get('/api/v61/scripts/catalog')">Scripts</button><h3>Import text file (max 8 KB)</h3><input id="name" placeholder="example.duck"><textarea id="content" placeholder="Paste script text"></textarea><button onclick="upload()">Import (no overwrite)</button><pre id="output"></pre><a href="/scripts">Script Manager</a> | <a href="/wifi">Wi-Fi Manager</a><script>const e=id=>document.getElementById(id);async function get(p){let r=await fetch(p,{headers:{'X-Cyberdeck-Token':e('token').value}});e('output').textContent='HTTP '+r.status+'\n'+await r.text()}async function upload(){let r=await fetch('/api/v61/scripts/import',{method:'POST',headers:{'X-Cyberdeck-Token':e('token').value,'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({name:e('name').value,content:e('content').value})});e('output').textContent='HTTP '+r.status+'\n'+await r.text()}</script>)V61HTML");});
}
}
