#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "v62_server.h"
#include "v61_scripts.h"
#include "v61_wifi.h"

// V6.1 stages 10-15: storage maintenance, backup, inventory and diagnostics.
// No execution, HID, SD pin access, or radio transmission.
namespace V61Mega {
static void routes(V62Server &s) {
 s.on("/scripts/storage", HTTP_GET, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  String j="{\"total_bytes\":"+String(LittleFS.totalBytes())+",\"used_bytes\":"+String(LittleFS.usedBytes())+",\"max_script_bytes\":"+String(V61Scripts::MAX_SCRIPT)+"}";
  s.send(200,"application/json",j);
 });
 s.on("/scripts/copy", HTTP_POST, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String src=s.arg("source"),dst=s.arg("destination");
  if(!V61Scripts::validName(src)||!V61Scripts::validName(dst)||src==dst){s.send(400,"text/plain","Invalid filenames");return;}
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  if(LittleFS.exists(V61Scripts::path(dst))){s.send(409,"text/plain","Destination exists");return;}
  File in=LittleFS.open(V61Scripts::path(src),"r");if(!in){s.send(404,"text/plain","Source missing");return;}
  if(in.size()>V61Scripts::MAX_SCRIPT){in.close();s.send(413,"text/plain","Source too large");return;}
  File out=LittleFS.open(V61Scripts::path(dst),"w");if(!out){in.close();s.send(507,"text/plain","Storage full");return;}
  size_t expected=in.size(),written=0;uint8_t buf[128];
  while(in.available()){size_t n=in.read(buf,sizeof(buf));if(!n)break;size_t w=out.write(buf,n);written+=w;if(w!=n)break;}
  in.close();out.close();
  if(written!=expected){LittleFS.remove(V61Scripts::path(dst));s.send(507,"text/plain","Copy incomplete");return;}
  s.send(200,"application/json","{\"ok\":true}");
 });
 s.on("/scripts/rename", HTTP_POST, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String src=s.arg("source"),dst=s.arg("destination");
  if(!V61Scripts::validName(src)||!V61Scripts::validName(dst)||src==dst){s.send(400,"text/plain","Invalid filenames");return;}
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  if(!LittleFS.exists(V61Scripts::path(src))){s.send(404,"text/plain","Source missing");return;}
  if(LittleFS.exists(V61Scripts::path(dst))){s.send(409,"text/plain","Destination exists");return;}
  bool ok=LittleFS.rename(V61Scripts::path(src),V61Scripts::path(dst));
  s.send(ok?200:500,"application/json",ok?"{\"ok\":true}":"{\"ok\":false}");
 });
 s.on("/scripts/backup", HTTP_GET, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  if(!V61Scripts::ready){s.send(503,"text/plain","LittleFS unavailable");return;}
  File dir=LittleFS.open("/scripts");if(!dir||!dir.isDirectory()){s.send(500,"text/plain","Scripts directory missing");return;}
  // JSON backup capped to keep memory use predictable.
  String j="{\"format\":\"cyberdeck-v61-backup-1\",\"files\":[";bool first=true;int count=0;
  File f=dir.openNextFile();
  while(f){
   String n=String(f.name());int slash=n.lastIndexOf('/');if(slash>=0)n=n.substring(slash+1);
   if(!f.isDirectory()&&V61Scripts::validName(n)&&f.size()<=V61Scripts::MAX_SCRIPT){
    if(++count>12){f.close();dir.close();s.send(413,"text/plain","Backup limited to 12 files");return;}
    String body=f.readString();if(j.length()+body.length()*2>45000){f.close();dir.close();s.send(413,"text/plain","Backup exceeds 45 KB budget");return;}
    if(!first)j+=",";first=false;
    j+="{\"name\":\""+V61WiFi::escapeJson(n)+"\",\"content\":\""+V61WiFi::escapeJson(body)+"\"}";
   }
   f=dir.openNextFile();
  }
  dir.close();j+="]}";
  s.sendHeader("Content-Disposition","attachment; filename=cyberdeck-scripts-backup.json");
  s.send(200,"application/json",j);
 });
 s.on("/tools/health", HTTP_GET, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String j="{\"uptime_ms\":"+String(millis())+",\"free_heap\":"+String(ESP.getFreeHeap())+",\"free_psram\":"+String(ESP.getFreePsram())+",\"wifi_rssi\":"+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+",\"sta_connected\":"+String(WiFi.status()==WL_CONNECTED?"true":"false")+",\"ap_clients\":"+String(WiFi.softAPgetStationNum())+",\"littlefs_ready\":"+String(V61Scripts::ready?"true":"false")+",\"hid_execution\":false,\"sd_untested\":true}";
  s.send(200,"application/json",j);
 });
 s.on("/tools/safety", HTTP_GET, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  s.send(200,"application/json","{\"script_execution\":false,\"usb_hid\":false,\"sd_mount\":false,\"nat_supported\":false,\"rf_tx\":false,\"external_modules\":\"unverified\"}");
 });
}
}
