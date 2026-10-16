#pragma once
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <vector>
// V61_NAT_STAGE3: NAT support is optional in the Arduino lwIP build.
#if defined(V61_IDF_NAT_BUILD) && V61_IDF_NAT_BUILD
  #include <lwip/lwip_napt.h>
  #define V61_NAPT_AVAILABLE 1
#else
  #define V61_NAPT_AVAILABLE 0
#endif
#include <WiFiClient.h>
#include <Wire.h>


// Wi-Fi manager. Credentials never leave NVS or appear in API responses.
namespace V61WiFi {
struct Network { String ssid, password; int priority; };
static std::vector<Network> known;
static Preferences prefs;
static String apSSID, apPassword, adminToken;
static unsigned long lastConnect=0;
static bool restartAP=false;
static String lastError;
static const int MAX_NETWORKS=8;
static bool natRequested=false, natActive=false;
static bool lastUplink=false;
static unsigned long lastNatCheck=0;
static String natMessage="NAT not checked";
static bool enableNat(){
#if V61_NAPT_AVAILABLE
  if(WiFi.status()!=WL_CONNECTED){natActive=false;natMessage="STA not connected";return false;}
  // ESP32 Arduino lwIP NAT: enable NAPT on AP interface address.
  ip_napt_enable((u32_t)WiFi.softAPIP(), 1);
  natActive=true;
  natMessage="NAPT enabled (traffic not yet verified)";
  return true;
#else
  natActive=false;
  natMessage="NAT unavailable in this firmware's lwIP build";
  return false;
#endif
}
static void disableNat(){
#if V61_NAPT_AVAILABLE
  if(natActive)ip_napt_enable((u32_t)WiFi.softAPIP(), 0);
#endif
  natActive=false;
}


static String escapeJson(const String &s){String r; for(size_t i=0;i<s.length();++i){char c=s[i]; if(c=='"'||c=='\\')r+='\\'; if(c=='\n')r+="\\n"; else if(c=='\r')r+="\\r"; else if((uint8_t)c>=32)r+=c;}return r;}
static String html(const String &s){String r=s;r.replace("&","&amp;");r.replace("<","&lt;");r.replace(">","&gt;");r.replace("\"","&quot;");r.replace("'","&#39;");return r;}
static String randomPassword(){char b[21];snprintf(b,sizeof(b),"%08lX%08lX%04lX",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)(esp_random()&0xffff));return String(b);}
static bool validSSID(const String &s){return s.length()>0&&s.length()<=32;}
static bool validPass(const String &s){return s.length()>=8&&s.length()<=63;}
static void load(){
 prefs.begin("v61wifi",false);
 apSSID=prefs.getString("ap_ssid",""); apPassword=prefs.getString("ap_pass","");
 if(!validSSID(apSSID)){char suf[7];snprintf(suf,sizeof(suf),"%06lX",(unsigned long)(ESP.getEfuseMac()&0xffffff));apSSID="CYBERDECK-"+String(suf);}
 if(!validPass(apPassword)){apPassword=randomPassword();prefs.putString("ap_pass",apPassword);}
 prefs.putString("ap_ssid",apSSID);
 adminToken=prefs.getString("admin","");if(adminToken.length()<16){adminToken=randomPassword();prefs.putString("admin",adminToken);}
 known.clear();int count=constrain(prefs.getInt("count",0),0,MAX_NETWORKS);
 for(int i=0;i<count;i++){String k="n"+String(i);Network n{prefs.getString((k+"s").c_str(),""),prefs.getString((k+"p").c_str(),""),prefs.getInt((k+"r").c_str(),0)};if(validSSID(n.ssid))known.push_back(n);}
}
static void save(){prefs.putInt("count",known.size());for(int i=0;i<MAX_NETWORKS;i++){String k="n"+String(i);if(i<(int)known.size()){prefs.putString((k+"s").c_str(),known[i].ssid);prefs.putString((k+"p").c_str(),known[i].password);prefs.putInt((k+"r").c_str(),known[i].priority);}else{prefs.remove((k+"s").c_str());prefs.remove((k+"p").c_str());prefs.remove((k+"r").c_str());}}}
static void start(){load();natRequested=prefs.getBool("nat",false);WiFi.mode(WIFI_AP_STA);WiFi.setAutoReconnect(true);WiFi.softAP(apSSID.c_str(),apPassword.c_str());}
static int nextCandidate=0;
static void connectBest(){
 if(WiFi.status()==WL_CONNECTED||known.empty())return;
 // Rotate through saved profiles; priority wins on each complete cycle.
 std::vector<int> order;for(int i=0;i<(int)known.size();i++)order.push_back(i);
 for(size_t i=0;i<order.size();i++)for(size_t j=i+1;j<order.size();j++)
   if(known[order[j]].priority>known[order[i]].priority){int tmp=order[i];order[i]=order[j];order[j]=tmp;}
 int index=order[nextCandidate%order.size()];nextCandidate=(nextCandidate+1)%order.size();
 WiFi.begin(known[index].ssid.c_str(),known[index].password.c_str());lastConnect=millis();
}
static bool upsert(const String &ssid,const String &pass,int priority){
 if(!validSSID(ssid)||(!pass.isEmpty()&&!validPass(pass)))return false;
 int idx=-1;for(int i=0;i<(int)known.size();i++)if(known[i].ssid==ssid)idx=i;
 if(idx<0&&(int)known.size()>=MAX_NETWORKS)return false;
 if(idx>=0){known[idx].priority=priority;if(pass.length())known[idx].password=pass;}
 else known.push_back({ssid,pass,priority});
 save();nextCandidate=0;lastConnect=0;return true;
}
static bool setAP(const String &ssid,const String &pass){
 if(!validSSID(ssid)||!validPass(pass))return false;
 apSSID=ssid;apPassword=pass;prefs.putString("ap_ssid",ssid);prefs.putString("ap_pass",pass);
 restartAP=true;return true;
}
static void tick(){
 if(restartAP){restartAP=false;disableNat();WiFi.softAPdisconnect(true);delay(80);WiFi.softAP(apSSID.c_str(),apPassword.c_str());lastNatCheck=0;}
 bool uplink=WiFi.status()==WL_CONNECTED;
 if(!uplink&&natActive)disableNat();
 if(natRequested&&uplink&&(!natActive||!lastUplink)&&millis()-lastNatCheck>1500){lastNatCheck=millis();enableNat();}
 if(!natRequested&&natActive)disableNat();
 lastUplink=uplink;
 if(WiFi.status()!=WL_CONNECTED&&(lastConnect==0||millis()-lastConnect>30000))connectBest();
}
static bool authorized(WebServer &s){String t=s.header("X-Cyberdeck-Token");if(t.length()==0)t=s.arg("token");return t.length()>=16&&t==adminToken;}
static void forbidden(WebServer &s){s.send(403,"application/json","{\"error\":\"admin token required\"}");}
static String status(){String j="{\"ap_ssid\":\""+escapeJson(apSSID)+"\",\"ap_ip\":\""+WiFi.softAPIP().toString()+"\",\"clients\":"+String(WiFi.softAPgetStationNum())+",\"sta_connected\":"+String(WiFi.status()==WL_CONNECTED?"true":"false")+",\"sta_ssid\":\""+escapeJson(WiFi.status()==WL_CONNECTED?WiFi.SSID():"")+"\",\"sta_ip\":\""+(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String(""))+"\",\"nat_active\":"+String(natActive?"true":"false")+",\"nat_supported\":"+String(V61_NAPT_AVAILABLE?"true":"false")+",\"nat_requested\":"+String(natRequested?"true":"false")+",\"nat_message\":\""+escapeJson(natMessage)+"\",\"networks\":[";
 for(int i=0;i<(int)known.size();i++){if(i)j+=",";j+="{\"ssid\":\""+escapeJson(known[i].ssid)+"\",\"priority\":"+String(known[i].priority)+"}";}return j+"]}";}
static void routes(WebServer &s){
 const char *headers[]={"X-Cyberdeck-Token"};s.collectHeaders(headers,1);
 s.on("/wifi/nat",HTTP_POST,[&s](){if(!authorized(s))return forbidden(s);String val=s.arg("enabled");if(val!="0"&&val!="1"){s.send(400,"text/plain","enabled must be 0 or 1");return;}natRequested=val=="1";prefs.putBool("nat",natRequested);if(!natRequested){disableNat();natMessage="NAT disabled";}else if(WiFi.status()==WL_CONNECTED){enableNat();}else natMessage="Waiting for STA connection";s.send(200,"application/json",status());});
 s.on("/wifi/tcp",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String host=s.arg("host");int port=s.arg("port").toInt();if(host.isEmpty()||host.length()>120||port<1||port>65535||host.indexOf('/')>=0){s.send(400,"text/plain","Invalid host or port");return;}if(WiFi.status()!=WL_CONNECTED){s.send(503,"text/plain","STA disconnected");return;}WiFiClient client;client.setTimeout(2000);unsigned long begin=millis();bool ok=client.connect(host.c_str(),(uint16_t)port,2000);unsigned long elapsed=millis()-begin;client.stop();s.send(ok?200:503,"application/json",String("{\"ok\":")+(ok?"true":"false")+",\"latency_ms\":"+String(elapsed)+"}");});
 // V61_EXPANSION_STAGE4: diagnostics, history, and safe hardware discovery.
 s.on("/wifi/overview",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String j=status();j.remove(j.length()-1);j+=",\"sta_rssi\":"+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+",\"sta_gateway\":\""+WiFi.gatewayIP().toString()+"\",\"sta_dns\":\""+WiFi.dnsIP().toString()+"\",\"ap_channel\":"+String(WiFi.channel())+",\"uptime_ms\":"+String(millis())+"}";s.send(200,"application/json",j);});
 s.on("/wifi/scan.csv",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);int n=WiFi.scanNetworks(false,true);String csv="ssid,bssid,rssi,channel,encryption\n";for(int i=0;i<n&&i<40;i++){String ssid=WiFi.SSID(i);ssid.replace("\"","\"\"");csv+="\""+ssid+"\","+WiFi.BSSIDstr(i)+","+String(WiFi.RSSI(i))+","+String(WiFi.channel(i))+","+String((int)WiFi.encryptionType(i))+"\n";}WiFi.scanDelete();s.sendHeader("Content-Disposition","attachment; filename=wifi-scan.csv");s.send(200,"text/csv",csv);});
 s.on("/hardware/i2c",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String j="[";int count=0;for(uint8_t a=0x08;a<=0x77;a++){Wire.beginTransmission(a);if(Wire.endTransmission()==0){if(count++)j+=",";j+=String(a);}}s.send(200,"application/json",j+"]");});
 s.on("/hardware/system",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String j="{\"heap_free\":"+String(ESP.getFreeHeap())+",\"psram_size\":"+String(ESP.getPsramSize())+",\"flash_size\":"+String(ESP.getFlashChipSize())+",\"uptime_ms\":"+String(millis())+"}";s.send(200,"application/json",j);});
 s.on("/wifi/status",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);s.send(200,"application/json",status());});
 s.on("/wifi/networks",HTTP_POST,[&s](){if(!authorized(s))return forbidden(s);String ssid=s.arg("ssid"),pass=s.arg("password");int pr=constrain(s.arg("priority").toInt(),-100,100);if(!validSSID(ssid)||(!pass.isEmpty()&&pass.length()<8)){s.send(400,"text/plain","Invalid SSID/password");return;}int idx=-1;for(int i=0;i<(int)known.size();i++)if(known[i].ssid==ssid)idx=i;if(idx<0&&(int)known.size()>=MAX_NETWORKS){s.send(409,"text/plain","Network list full");return;}if(idx>=0){known[idx].priority=pr;if(pass.length())known[idx].password=pass;}else known.push_back({ssid,pass,pr});save();WiFi.disconnect(false,false);lastConnect=0;s.send(200,"application/json","{\"ok\":true}");});
 s.on("/wifi/delete",HTTP_POST,[&s](){if(!authorized(s))return forbidden(s);String ssid=s.arg("ssid");for(auto it=known.begin();it!=known.end();++it)if(it->ssid==ssid){known.erase(it);save();s.send(200,"application/json","{\"ok\":true}");return;}s.send(404,"text/plain","Not found");});
 s.on("/wifi/ap",HTTP_POST,[&s](){if(!authorized(s))return forbidden(s);String ssid=s.arg("ssid"),pass=s.arg("password");if(!validSSID(ssid)||!validPass(pass)){s.send(400,"text/plain","SSID 1-32 chars, password 8-63 chars");return;}apSSID=ssid;apPassword=pass;prefs.putString("ap_ssid",ssid);prefs.putString("ap_pass",pass);s.send(200,"text/plain","Saved. AP will restart; reconnect with new credentials.");restartAP=true;});
 s.on("/wifi/scan",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);int n=WiFi.scanNetworks(false,true);String j="[";for(int i=0;i<n&&i<40;i++){if(i)j+=",";j+="{\"ssid\":\""+escapeJson(WiFi.SSID(i))+"\",\"bssid\":\""+WiFi.BSSIDstr(i)+"\",\"rssi\":"+String(WiFi.RSSI(i))+",\"channel\":"+String(WiFi.channel(i))+",\"encryption\":"+String((int)WiFi.encryptionType(i))+"}";}WiFi.scanDelete();s.send(200,"application/json",j+"]");});
 s.on("/wifi/dns",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String host=s.arg("host");if(host.length()==0||host.length()>120){s.send(400,"text/plain","Invalid host");return;}IPAddress ip;bool ok=WiFi.status()==WL_CONNECTED&&WiFi.hostByName(host.c_str(),ip);s.send(ok?200:503,"application/json",String("{\"ok\":")+(ok?"true":"false")+",\"ip\":\""+(ok?ip.toString():String(""))+"\"}");});
 s.on("/wifi/http",HTTP_GET,[&s](){if(!authorized(s))return forbidden(s);String url=s.arg("url");if(WiFi.status()!=WL_CONNECTED||!url.startsWith("http://")||url.length()>200){s.send(400,"text/plain","Requires STA and http:// URL");return;}HTTPClient client;client.setTimeout(3500);client.begin(url);int code=client.sendRequest("HEAD");client.end();s.send(200,"application/json","{\"http_status\":"+String(code)+"}");});
 s.on("/wifi",HTTP_GET,[&s](){String h=R"CYBERDECK(<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'><style>body{background:#081627;color:#dff;font:16px sans-serif;max-width:680px;margin:auto;padding:18px}input,button{padding:11px;margin:5px;background:#152a40;color:white;border:1px solid #0cf;border-radius:7px}section{border:1px solid #0cf;border-radius:10px;padding:14px;margin:12px 0}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h1>V6.1 Wi-Fi Manager</h1><p>Enter admin token shown on Cyberdeck settings screen. Keep it private.</p><input id=t type=password placeholder='Admin token'><button onclick='refresh()'>Status</button><section><pre id=state></pre></section><section><h3>Add / update home Wi-Fi</h3><input id=ssid placeholder='SSID'><input id=pass type=password placeholder='Password'><input id=priority type=number value=10 placeholder='Priority'><button onclick='post("/wifi/networks",{ssid:ssid.value,password:pass.value,priority:priority.value})'>Save</button><button onclick='post("/wifi/delete",{ssid:ssid.value})'>Delete</button></section><section><h3>Cyberdeck hotspot</h3><input id=apssid placeholder='New AP name'><input id=appass type=password placeholder='New AP password (8+ chars)'><button onclick='post("/wifi/ap",{ssid:apssid.value,password:appass.value})'>Change AP</button></section><section><button onclick='get("/wifi/scan")'>Scan networks</button><button onclick='get("/wifi/overview")'>Overview</button><button onclick='get("/hardware/system")'>System</button><button onclick='get("/hardware/i2c")'>I2C</button><button onclick='get("/wifi/scan.csv")'>CSV text</button><input id=host placeholder='example.com'><button onclick='get("/wifi/dns?host="+encodeURIComponent(host.value))'>DNS lookup</button></section><pre id=output></pre><a href='/'>Home</a><script>const tok=()=>document.getElementById("t").value;async function get(p){let r=await fetch(p,{headers:{"X-Cyberdeck-Token":tok()}});output.textContent=await r.text()}async function refresh(){let r=await fetch("/wifi/status",{headers:{"X-Cyberdeck-Token":tok()}});state.textContent=await r.text()}async function post(p,o){let r=await fetch(p,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded","X-Cyberdeck-Token":tok()},body:new URLSearchParams(o)});output.textContent=await r.text();if(r.ok&&p!=="/wifi/ap")refresh()}</script>)CYBERDECK";s.send(200,"text/html",h);});
}
} // namespace V61WiFi
