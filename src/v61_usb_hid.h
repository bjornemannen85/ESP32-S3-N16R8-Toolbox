#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "v61_wifi.h"
// Experimental native USB HID. OFF by default until physical USB and stop test.
#ifndef V61_ENABLE_NATIVE_HID
#define V61_ENABLE_NATIVE_HID 0
#endif
#if V61_ENABLE_NATIVE_HID
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#endif
namespace V61USBHID {
static bool enabled=false;
#if V61_ENABLE_NATIVE_HID
static USBHIDKeyboard keyboard;
static USBHIDMouse mouse;
static bool initialized=false;
static bool begin(){
 if(initialized)return true;
 keyboard.begin();mouse.begin();USB.begin();initialized=true;return true;
}
#endif
static void stop(){
#if V61_ENABLE_NATIVE_HID
 if(initialized){keyboard.releaseAll();mouse.release(MOUSE_ALL);}
#endif
 enabled=false;
}
static String status(){
 return String("{\"enabled\":")+(enabled?"true":"false")+
  ",\"native_hid_compiled\":"+(V61_ENABLE_NATIVE_HID?"true":"false")+
  ",\"hardware_tested\":false,\"reason\":\"Physical USB emergency-stop validation required before enabling\"}";
}
static void routes(WebServer &s){
 s.on("/api/v61/usb-hid/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",status());});
 s.on("/api/v61/usb-hid/start",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(423,"application/json",status());});
 s.on("/api/v61/usb-hid/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);stop();s.send(200,"application/json",status());});
 s.on("/toolbox/usb-hid",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"PAGE(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck USB HID</title><style>body{font:16px sans-serif;background:#071729;color:#def;max-width:680px;margin:auto;padding:18px}input,button{padding:10px;margin:5px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap}</style><h2>USB HID – native stack staging</h2><p>Native keyboard/mouse code is gated OFF at build time. No automatic input. Activation requires a verified physical emergency stop and USB configuration.</p><input id=t type=password placeholder="Admin token"><button onclick="go('status','GET')">Status</button><button onclick="go('stop','POST')">STOP</button><pre id=o></pre><script>async function go(x,m){let r=await fetch('/api/v61/usb-hid/'+x,{method:m,headers:{'X-Cyberdeck-Token':t.value}});o.textContent=r.status+' '+await r.text()}</script>)PAGE");});
}
}
