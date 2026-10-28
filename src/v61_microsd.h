#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include "v62_server.h"
#include "v61_wifi.h"
// ES3C28P candidate SDMMC mapping; no mount on boot.
namespace V61MicroSD {
static bool mounted=false;
static String lastError="not mounted";
static const int CLK=38,CMD=40,D0=39,D1=41,D2=48,D3=47;
static String json(){
  return String("{\"mounted\":")+(mounted?"true":"false")+
    ",\"capacity_bytes\":"+(mounted?String((unsigned long long)SD_MMC.cardSize()):"0")+
    ",\"used_bytes\":"+(mounted?String((unsigned long long)SD_MMC.usedBytes()):"0")+
    ",\"error\":\""+V61WiFi::escapeJson(lastError)+"\",\"mode\":\"1-bit\",\"pins_verified_on_hardware\":false}";
}
static void unmount(){if(mounted){SD_MMC.end();mounted=false;}lastError="unmounted";}
static bool mount(){
 if(mounted)return true;
 // Arduino-ESP32 2.x supports setPins for ESP32-S3 SDMMC routing.
 if(!SD_MMC.setPins(CLK,CMD,D0,D1,D2,D3)){lastError="SDMMC pin assignment rejected";return false;}
 if(!SD_MMC.begin("/sdcard",true,false,SDMMC_FREQ_DEFAULT,5)){lastError="SDMMC mount failed (check card and wiring)";return false;}
 if(SD_MMC.cardType()==CARD_NONE){SD_MMC.end();lastError="no card detected";return false;}
 mounted=true;lastError="ok";return true;
}
static bool safePath(const String &p){if(p.length()<1||p.length()>96||p[0]!='/'||p.indexOf("..")>=0||p.indexOf('\\')>=0)return false;for(char c:p)if((unsigned char)c<32)return false;return true;}
static void routes(V62Server &s){
 s.on("/api/v61/sd/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",json());});
 s.on("/api/v61/sd/mount",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);bool ok=mount();s.send(ok?200:503,"application/json",json());});
 s.on("/api/v61/sd/unmount",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);unmount();s.send(200,"application/json",json());});
 s.on("/api/v61/sd/list",HTTP_GET,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  if(!mounted){s.send(409,"text/plain","SD not mounted");return;}
  String p=s.arg("path");if(!p.length())p="/";
  if(!safePath(p)){s.send(400,"text/plain","Invalid path");return;}
  File dir=SD_MMC.open(p);if(!dir||!dir.isDirectory()){s.send(404,"text/plain","Directory not found");return;}
  String out="{\"path\":\""+V61WiFi::escapeJson(p)+"\",\"entries\":[";
  File f=dir.openNextFile();int count=0;
  while(f&&count<60){if(count++)out+=",";out+="{\"name\":\""+V61WiFi::escapeJson(String(f.name()))+"\",\"directory\":"+(f.isDirectory()?String("true"):String("false")) + ",\"bytes\":"+String((unsigned long)f.size())+"}";f.close();f=dir.openNextFile();}
  dir.close();out+="]}";s.send(200,"application/json",out);
 });
 s.on("/toolbox/sd",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck microSD</title><style>body{background:#081627;color:#def;font:16px sans-serif;max-width:700px;margin:auto;padding:18px}input,button{padding:10px;margin:4px;background:#18324b;color:#fff;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h2>microSD (experimental)</h2><p>Manual mount only. 1-bit SDMMC GPIO38/40/39. Pin map requires hardware validation. Read-only browser; no upload, delete or format.</p><button onclick="req('status')">Status</button><button onclick="req('mount')">Mount</button><button onclick="req('unmount')">Unmount</button><p><input id=p value="/" placeholder="Directory"><button onclick="req('list?path='+encodeURIComponent(p.value))">List files</button></p><pre id=o></pre><script>async function req(op){let post=op==='mount'||op==='unmount';let r=await fetch('/api/v61/sd/'+op,{method:post?'POST':'GET',headers:{}});o.textContent=r.status+' '+await r.text()}</script>)HTML");});
}
}
