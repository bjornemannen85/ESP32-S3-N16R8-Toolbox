#pragma once
String v62LiveFeatures();
#include <Arduino.h>
#include "v62_server.h"
#include <WiFi.h>
#include "v61_wifi.h"
#include "v61_bundle81100.h"

// V6.1 milestone 101-120: manually requested ADC sampling and UART emergency stop.
// No GPIO output, USB HID, BLE GATT transfer or unverified SD pin activation.
namespace V61Bundle101120 {
static bool (*modulePinFree)(int)=nullptr;
static bool adcPinAllowed(int p) {
  if(modulePinFree&&!modulePinFree(p))return false;
  // ADC1 GPIO 1-10 on ESP32-S3; exclude known LCD/strap/board pins.
  // Actual physical accessibility MUST be verified before sampling.
  return (p==1 || p==2 || p==4 || p==5 || p==6 || p==7 );
}
static void routes(V62Server &s) {
  s.on("/api/v61/adc/capabilities",HTTP_GET,[&s](){
    if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
    s.send(200,"application/json","{\"pins\":[1,2,4,5,6,7],\"manual_only\":true,\"output_enabled\":false,\"board_pins_verified\":false,\"voltage_limit\":\"3.3V maximum\"}");
  });
  s.on("/api/v61/adc/read",HTTP_GET,[&s](){
    if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
    String arg=s.arg("pin");
    if(!arg.length()||arg.length()>2){s.send(400,"text/plain","Specify valid ADC1 pin");return;}
    for(size_t i=0;i<arg.length();i++)if(arg[i]<'0'||arg[i]>'9'){s.send(400,"text/plain","Invalid pin");return;}
    int p=arg.toInt();
    if(!adcPinAllowed(p)){s.send(400,"text/plain","Pin not permitted");return;}
    // Explicit request only; never sample automatically.
    pinMode(p,INPUT);
    int raw=analogRead(p);
    s.send(200,"application/json",String("{\"pin\":")+p+",\"raw\":"+raw+",\"bits\":12,\"calibrated\":false,\"hardware_tested\":false}");
  });
  s.on("/api/v61/safety/stop",HTTP_POST,[&s](){
    if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
    if(V61Bundle81100::active){V61Bundle81100::port.end();V61Bundle81100::active=false;}
    s.send(200,"application/json","{\"uart_stopped\":true,\"hid_active\":false,\"ble_transfer_active\":false}");
  });
  s.on("/api/v61/roadmap",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",v62LiveFeatures());
  });
  s.on("/toolbox/io",HTTP_GET,[&s](){
    s.send(200,"text/html; charset=utf-8",R"PAGE(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck IO Tools</title><style>body{background:#071729;color:#def;font:16px sans-serif;max-width:700px;margin:auto;padding:16px}button,input,select{padding:10px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px;margin:5px}pre{white-space:pre-wrap}a{color:#4df}</style><h2>Cyberdeck I/O Lab</h2><p>Hardware pins are not verified. Do not connect voltages above 3.3 V. ADC sampling is manual only. No output pins enabled.</p><input id="token" type="password" placeholder="Admin token"><p><label>ADC1 GPIO <select id="pin"><option>1</option><option>2</option><option>4</option><option>5</option><option>6</option><option>7</option></select></label><button onclick="get('/api/v61/adc/read?pin='+pin.value)">Read ADC</button></p><button onclick="post('/api/v61/safety/stop')">STOP UART</button><button onclick="get('/api/v61/roadmap')">Feature status</button><button onclick="get('/api/v61/adc/capabilities')">ADC capabilities</button><pre id="result"></pre><p><a href="/toolbox/uart">UART Lab</a> | <a href="/toolbox/operations">Operations</a></p><script>const headers=()=>({'X-Cyberdeck-Token':document.getElementById('token').value});async function get(url){const r=await fetch(url,{headers:headers()});result.textContent=r.status+' '+await r.text()}async function post(url){const r=await fetch(url,{method:'POST',headers:headers()});result.textContent=r.status+' '+await r.text()}</script>)PAGE");
  });
}
}
