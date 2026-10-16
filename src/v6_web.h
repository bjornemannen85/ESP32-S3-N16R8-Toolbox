#ifndef CYBERDECK_V6_WEB_H
#define CYBERDECK_V6_WEB_H
#include <WebServer.h>
#include <esp_system.h>
WebServer v6Server(80);
String v6SSID, v6Password;
String v6Events[12];
uint8_t v6EventPos=0;
void v6Log(const String &msg){v6Events[v6EventPos++%12]=String(millis()/1000)+"s "+msg;}
String v6HtmlEscape(String s){s.replace("&","&amp;");s.replace("<","&lt;");s.replace(">","&gt;");s.replace("\"","&quot;");return s;}
void v6Start(){
 uint64_t mac=ESP.getEfuseMac();
 char suffix[7];snprintf(suffix,sizeof(suffix),"%06lX",(unsigned long)(mac&0xFFFFFF));
 v6SSID=String("CYBERDECK-")+suffix;
 char pass[17];uint32_t a=esp_random(),b=esp_random();
 snprintf(pass,sizeof(pass),"%08lX%08lX",(unsigned long)a,(unsigned long)b);
 v6Password=pass;
 WiFi.mode(WIFI_AP_STA);
 if(!WiFi.softAP(v6SSID.c_str(),v6Password.c_str())){v6Log("AP start FAILED");return;}
 v6Log("AP started");
 v6Server.on("/",HTTP_GET,[](){
  String h="<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'>";
  h+="<style>body{background:#07152b;color:#dff;font:16px sans-serif;padding:18px}h1{color:#00dfff}section{border:1px solid #00dfff;border-radius:12px;padding:15px;margin:12px 0}a{color:#00dfff}</style>";
  h+="<h1>CYBERDECK V6.0 ULTIMATE</h1><section><h2>System</h2>CPU: "+String(ESP.getCpuFreqMHz())+" MHz<br>Free heap: "+String(ESP.getFreeHeap()/1024)+" KB<br>PSRAM: "+String(ESP.getPsramSize()/1048576)+" MB<br>Flash: "+String(ESP.getFlashChipSize()/1048576)+" MB<br>Uptime: "+String(millis()/1000)+" s</section>";
  h+="<section><h2>Radio</h2>Wi-Fi and BLE scans are available on the touchscreen. This dashboard is read-only.</section>";
  h+="<section><h2>Modules</h2>PN532, CC1101, IR, UART, GPIO and SD: NOT CONFIGURED</section>";
  h+="<section><h2>Events</h2><a href='/logs'>View log</a></section>";
  v6Server.send(200,"text/html",h);
 });
 v6Server.on("/logs",HTTP_GET,[](){
  String h="<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><h2>Cyberdeck events</h2><pre>";
  for(int i=0;i<12;i++){int j=(v6EventPos+i)%12;if(v6Events[j].length())h+=v6HtmlEscape(v6Events[j])+"\n";}
  h+="</pre><a href='/'>Back</a>";v6Server.send(200,"text/html",h);
 });
 v6Server.on("/status",HTTP_GET,[](){
  String j="{\"version\":\"6.0\",\"heap_kb\":"+String(ESP.getFreeHeap()/1024)+",\"uptime_s\":"+String(millis()/1000)+"}";
  v6Server.send(200,"application/json",j);
 });
 v6Server.begin();v6Log("Web server ready");
}
#endif
