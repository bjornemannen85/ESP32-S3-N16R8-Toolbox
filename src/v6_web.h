#ifndef CYBERDECK_V6_WEB_H
#define CYBERDECK_V6_WEB_H
#include "v62_server.h"
#include <esp_system.h>
V62Server v6Server(80);
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
  v6Server.sendHeader("Location","/access");v6Server.send(302,"text/plain","Cyberdeck V6.2 Full Access");
 });
 v6Server.on("/logs",HTTP_GET,[](){
  String h="<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><h2>Cyberdeck events</h2><pre>";
  for(int i=0;i<12;i++){int j=(v6EventPos+i)%12;if(v6Events[j].length())h+=v6HtmlEscape(v6Events[j])+"\n";}
  h+="</pre><a href='/'>Back</a>";v6Server.send(200,"text/html",h);
 });
 v6Server.on("/status",HTTP_GET,[](){
  String j="{\"version\":\"6.2\",\"heap_kb\":"+String(ESP.getFreeHeap()/1024)+",\"uptime_s\":"+String(millis()/1000)+"}";
  v6Server.send(200,"application/json",j);
 });
 v6Server.begin();v6Log("Web server ready");
}
#endif
