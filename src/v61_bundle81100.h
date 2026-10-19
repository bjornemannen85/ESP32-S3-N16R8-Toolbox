#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "v62_server.h"
#include <WiFi.h>
#include "v61_wifi.h"

// V6.1 81-100: opt-in UART diagnostics. No pins are activated on boot.
// Pin map is intentionally conservative until the physical board is verified.
namespace V61Bundle81100 {
static HardwareSerial port(1);
static bool active=false;
static int rxPin=-1, txPin=-1;
static uint32_t baud=4800;
static String capture;
static const size_t CAPTURE_MAX=2048;
static unsigned long received=0, transmitted=0;
static bool pinAllowed(int p){
  // Do not use flash/PSRAM, boot strapping, onboard LCD, touch, USB, or SD candidate pins.
  if(p<1||p>48||(p>=26&&p<=37))return false;
  const int reserved[]={0,3,10,11,12,13,15,16,18,19,20,38,39,40,41,45,46,47,48};
  for(int x:reserved)if(p==x)return false;
  return true;
}
static void tick(){
  if(!active)return;
  int budget=128;
  while(budget-- > 0 && port.available()>0){
    uint8_t b=(uint8_t)port.read();
    char hex[4];snprintf(hex,sizeof(hex),"%02X ",b);
    if(capture.length()+3>CAPTURE_MAX)capture.remove(0,3);
    capture+=hex;++received;
  }
}
static String state(){
  return String("{\"active\":")+(active?"true":"false")+
    ",\"rx\":"+String(rxPin)+",\"tx\":"+String(txPin)+
    ",\"baud\":"+String(baud)+",\"received\":"+String(received)+
    ",\"transmitted\":"+String(transmitted)+
    ",\"capture_hex\":\""+capture+"\",\"hardware_tested\":false}";
}
static void routes(V62Server &s){
  s.on("/api/v61/uart/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",state());});
  s.on("/api/v61/uart/start",HTTP_POST,[&s](){
    if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
    int rx=s.arg("rx").toInt(),tx=s.arg("tx").toInt();
    uint32_t b=(uint32_t)s.arg("baud").toInt();
    if(!pinAllowed(rx)||!pinAllowed(tx)||rx==tx||!(b==1200||b==2400||b==4800||b==9600||b==19200||b==38400||b==57600||b==115200)){
      s.send(400,"text/plain","Invalid pins or baud. Verify board wiring before starting.");return;
    }
    if(active){port.end();active=false;}
    rxPin=rx;txPin=tx;baud=b;capture="";received=transmitted=0;
    port.begin(baud,SERIAL_8N1,rxPin,txPin);active=true;
    s.send(200,"application/json",state());
  });
  s.on("/api/v61/uart/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);if(active){port.end();active=false;}s.send(200,"application/json",state());});
  s.on("/api/v61/uart/clear",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);capture="";received=transmitted=0;s.send(200,"application/json",state());});
  s.on("/api/v61/uart/send",HTTP_POST,[&s](){
    if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
    if(!active){s.send(409,"text/plain","UART not started");return;}
    String data=s.arg("data");if(data.length()>128){s.send(413,"text/plain","Maximum 128 bytes per request");return;}
    size_t n=port.write((const uint8_t*)data.c_str(),data.length());transmitted+=n;
    s.send(n==data.length()?200:500,"application/json",String("{\"sent\":")+String(n)+"}");
  });
  s.on("/toolbox/uart",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck UART Lab</title><style>body{font:16px sans-serif;background:#071729;color:#def;max-width:700px;margin:auto;padding:16px}input,button{padding:10px;margin:4px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h2>UART Lab – manual control</h2><p>Warning: GPIO wiring is NOT verified. Use only with compatible 3.3V UART equipment. No UART pins are activated on boot.</p><input id=t type=password placeholder="Admin token"><p>RX <input id=rx type=number value=4 min=1 max=48> TX <input id=tx type=number value=5 min=1 max=48> Baud <select id=b><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option><option>1200</option><option>2400</option></select></p><button onclick="post('/api/v61/uart/start',{rx:rx.value,tx:tx.value,baud:b.value})">Start manually</button><button onclick="post('/api/v61/uart/stop',{})">STOP UART</button><button onclick="post('/api/v61/uart/clear',{})">Clear</button><button onclick="get()">Read status</button><p><input id=d placeholder="Text to send (max 128 bytes)"><button onclick="post('/api/v61/uart/send',{data:d.value})">Send</button></p><pre id=o></pre><a href="/toolbox/operations">Operations</a><script>const h=()=>({'X-Cyberdeck-Token':document.getElementById('t').value});async function get(){let r=await fetch('/api/v61/uart/status',{headers:h()});o.textContent=r.status+' '+await r.text()}async function post(url,params){let r=await fetch(url,{method:'POST',headers:{...h(),'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(params)});o.textContent=r.status+' '+await r.text()}</script>)HTML");});
  s.on("/api/v61/uart/capabilities",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json","{\"manual_start\":true,\"stop\":true,\"text_tx\":true,\"hex_rx\":true,\"auto_run\":false,\"hid\":false,\"ble_file_transfer\":false,\"hardware_tested\":false}");});
}
}
