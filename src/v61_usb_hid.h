#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "v61_wifi.h"
// USB HID is intentionally held behind a hardware-validation gate.
// No USB descriptors, keyboard reports or mouse reports are started.
namespace V61USBHID {
static bool enabled=false;
static String status(){return "{\"enabled\":false,\"supported_in_build\":false,\"hardware_tested\":false,\"reason\":\"USB HID requires native USB stack and physical stop validation\"}";}
static void stop(){enabled=false;}
static void routes(WebServer &s){
 s.on("/api/v61/usb-hid/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",status());});
 s.on("/api/v61/usb-hid/start",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(501,"application/json",status());});
 s.on("/api/v61/usb-hid/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);stop();s.send(200,"application/json",status());});
 s.on("/toolbox/usb-hid",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"PAGE(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><title>USB HID - staging</title><style>body{font:16px sans-serif;background:#071729;color:#def;max-width:680px;margin:auto;padding:18px}input,button{padding:10px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap}</style><h2>USB HID – development gate</h2><p>USB HID is NOT enabled in this build. No automatic keyboard/mouse actions. Native USB and a physical emergency stop must be tested first.</p><input id=t type=password placeholder="Admin token"><button onclick="go('status','GET')">Status</button><button onclick="go('stop','POST')">STOP</button><pre id=o></pre><script>async function go(x,m){let r=await fetch('/api/v61/usb-hid/'+x,{method:m,headers:{'X-Cyberdeck-Token':t.value}});o.textContent=r.status+' '+await r.text()}</script>)PAGE");});
}
}
