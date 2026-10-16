#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <LittleFS.h>
#include "v61_wifi.h"
#include "v61_scripts.h"
// Explicitly started BLE GATT receiver. No auto-start and no script execution.
// One transfer at a time, <=8192 bytes, CRC32 verified before commit.
namespace V61BleTransfer {
static bool running=false,receiving=false;
static BLEServer *server=nullptr;
static BLECharacteristic *dataChar=nullptr;
static String name;
static String payload;
static uint32_t expectedCRC=0;
static size_t expectedSize=0;
static String lastResult="idle";
static uint32_t crc32(const String &s){uint32_t c=0xFFFFFFFF;for(size_t i=0;i<s.length();++i){c^=(uint8_t)s[i];for(int b=0;b<8;b++)c=(c>>1)^((c&1)?0xEDB88320UL:0);}return ~c;}
static void reset(){receiving=false;name="";payload="";expectedSize=0;expectedCRC=0;}
static void command(const std::string &value){
  if(value.size()>512){lastResult="chunk too large";reset();return;}
  String v;v.reserve(value.size());for(char c:value)v+=(char)c;
  if(v=="CANCEL"){reset();lastResult="cancelled";return;}
  if(v.startsWith("BEGIN|")){
    reset();int a=v.indexOf('|',6),b=a<0?-1:v.indexOf('|',a+1);
    if(a<0||b<0){lastResult="bad BEGIN";return;}
    name=v.substring(6,a);String len=v.substring(a+1,b),crc=v.substring(b+1);
    for(char c:len)if(c<'0'||c>'9'){lastResult="bad length";return;}
    if(crc.length()!=8){lastResult="bad crc";return;}
    for(char c:crc)if(!isxdigit((unsigned char)c)){lastResult="bad crc";return;}
    unsigned long n=strtoul(len.c_str(),nullptr,10);
    if(!V61Scripts::ready||!V61Scripts::validName(name)||n>8192||n==0){lastResult="invalid name/size or filesystem";return;}
    expectedSize=n;expectedCRC=strtoul(crc.c_str(),nullptr,16);payload.reserve(n);receiving=true;lastResult="receiving";return;
  }
  if(v=="END"){
    if(!receiving||payload.length()!=expectedSize||crc32(payload)!=expectedCRC){lastResult="size/crc mismatch";reset();return;}
    String dest=V61Scripts::path(name);
    if(LittleFS.exists(dest)){lastResult="file already exists";reset();return;}
    File f=LittleFS.open(dest,"w");if(!f){lastResult="file open failed";reset();return;}
    size_t wrote=f.write((const uint8_t*)payload.c_str(),payload.length());f.close();
    if(wrote!=expectedSize){LittleFS.remove(dest);lastResult="write failed";}else lastResult="saved: "+name;
    reset();return;
  }
  if(!receiving){lastResult="send BEGIN first";return;}
  // Binary chunks use DATA: prefix; reserved control commands cannot be mistaken for data.
  if(value.size()<5||value.substr(0,5)!="DATA:"){lastResult="unknown command";reset();return;}
  size_t n=value.size()-5;
  if(payload.length()+n>expectedSize){lastResult="size exceeded";reset();return;}
  for(size_t i=5;i<value.size();++i)payload+=(char)value[i];
}
class Receiver:public BLECharacteristicCallbacks {void onWrite(BLECharacteristic *c) override {command(c->getValue());}};
static Receiver receiver;
static bool start(){
 if(running)return true;
 if(!V61Scripts::ready){lastResult="LittleFS unavailable";return false;}
 BLEDevice::init("Cyberdeck-V61-Transfer");
 server=BLEDevice::createServer();
 BLEService *svc=server->createService("8efbf6f0-5a0b-4f44-83a8-85b615ad0100");
 dataChar=svc->createCharacteristic("8efbf6f0-5a0b-4f44-83a8-85b615ad0101",BLECharacteristic::PROPERTY_WRITE);
 dataChar->setCallbacks(&receiver);svc->start();
 BLEAdvertising *adv=BLEDevice::getAdvertising();adv->addServiceUUID(svc->getUUID());adv->start();
 running=true;lastResult="advertising";return true;
}
static void stop(){if(!running)return;BLEDevice::getAdvertising()->stop();BLEDevice::deinit(true);server=nullptr;dataChar=nullptr;running=false;reset();lastResult="stopped";}
static String status(){return String("{\"enabled\":")+(running?"true":"false")+",\"receiving\":"+(receiving?"true":"false")+",\"bytes\":"+String(payload.length())+",\"expected\":"+String(expectedSize)+",\"result\":\""+V61WiFi::escapeJson(lastResult)+"\",\"hardware_tested\":false}";}
static void routes(WebServer &s){
 s.on("/api/v61/ble-transfer/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",status());});
 s.on("/api/v61/ble-transfer/start",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);bool ok=start();s.send(ok?200:503,"application/json",status());});
 s.on("/api/v61/ble-transfer/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);stop();s.send(200,"application/json",status());});
 s.on("/toolbox/ble-transfer",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck BLE Transfer</title><style>body{font:16px sans-serif;background:#071729;color:#def;max-width:680px;margin:auto;padding:18px}button,input{padding:10px;margin:6px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap}</style><h2>BLE file receiver (experimental)</h2><p>Manual start only. Max 8192 bytes. Saved files are NOT executed. Test only with your own paired/trusted devices. BLE writes currently have no application-layer authentication; disable the receiver when not in use.</p><input id=t type=password placeholder="Admin token"><button onclick="req('status')">Status</button><button onclick="req('start')">Enable receiver</button><button onclick="req('stop')">STOP receiver</button><pre id=o></pre><p>Service 8efbf6f0-5a0b-4f44-83a8-85b615ad0100, writable characteristic ...0101. Protocol: BEGIN|filename.txt|length|CRC32HEX, DATA:binary chunk, END or CANCEL.</p><script>async function req(op){let r=await fetch('/api/v61/ble-transfer/'+op,{method:op==='status'?'GET':'POST',headers:{'X-Cyberdeck-Token':t.value}});o.textContent=r.status+' '+await r.text()}</script>)HTML");});
}
}
