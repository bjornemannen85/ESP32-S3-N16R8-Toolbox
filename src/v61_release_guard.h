#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "v61_wifi.h"
#include "v61_ble_transfer.h"
#include "v61_bundle81100.h"
#include "v61_microsd.h"

// Security-oriented release staging. USB HID remains disabled until
// compatible USB stack and physical stop mechanism have been verified.
namespace V61ReleaseGuard {
static void routes(WebServer &s) {
 s.on("/api/v61/release/status", HTTP_GET, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String out=String("{\"ble_receiver\":")+(V61BleTransfer::running?"true":"false")+
   ",\"uart_active\":"+(V61Bundle81100::active?"true":"false")+
   ",\"usb_hid\":false,\"nat\":false,\"sd_mounted\":"+(V61MicroSD::mounted?"true":"false")+",\"sd_pins_verified\":false,\"physical_tested\":false}";
  s.send(200,"application/json",out);
 });
 s.on("/api/v61/release/stop", HTTP_POST, [&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  V61BleTransfer::stop();
  if(V61Bundle81100::active){V61Bundle81100::port.end();V61Bundle81100::active=false;}
  s.send(200,"application/json","{\"ble_stopped\":true,\"uart_stopped\":true,\"hid_enabled\":false}");
 });
 s.on("/toolbox/release-safety", HTTP_GET, [&s](){
  s.send(200,"text/html; charset=utf-8",R"PAGE(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck Release Safety</title><style>body{background:#071729;color:#def;font:16px sans-serif;max-width:680px;margin:auto;padding:18px}button,input{padding:12px;margin:5px;background:#18324b;color:white;border:1px solid #4cf;border-radius:6px}pre{white-space:pre-wrap}</style><h2>Release Safety</h2><p>BLE transfer requires a session code. USB HID and NAT remain disabled. SDMMC is manual-only and not hardware verified.</p><input id=t type=password placeholder="Admin token"><button onclick="req('/api/v61/release/status','GET')">Status</button><button onclick="req('/api/v61/ble-transfer/code','GET')">BLE session code</button><button onclick="req('/api/v61/release/stop','POST')">STOP BLE + UART</button><pre id=o></pre><a href="/toolbox/ble-transfer">BLE receiver</a><script>async function req(u,m){let r=await fetch(u,{method:m,headers:{'X-Cyberdeck-Token':t.value}});o.textContent=r.status+' '+await r.text()}</script>)PAGE");
 });
}
}
