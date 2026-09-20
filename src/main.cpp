#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>
#include <esp_chip_info.h>

// ESP32-S3 N16R8 CYBERDECK TOOLBOX V2
// Safe core firmware. External-radio modules are shown as expansion slots
// until the relevant hardware is connected and configured.

WebServer server(80);

static const char *AP_SSID = "S3-Cyberdeck";
static const char *AP_PASS = "cyberdeck123";

static const int I2C_SDA = 8;
static const int I2C_SCL = 9;

unsigned long bootMs;

String esc(String s) {
  s.replace("&","&amp;"); s.replace("<","&lt;");
  s.replace(">","&gt;"); s.replace("\"","&quot;");
  return s;
}

String head(const String &sub) {
  String h;
  h.reserve(5000);
  h += F("<!doctype html><html><head><meta charset='utf-8'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>S3 Cyberdeck</title><style>"
         "*{box-sizing:border-box}body{margin:0;background:#090b0e;color:#e8edf2;"
         "font-family:system-ui,Arial}.w{max-width:980px;margin:auto;padding:18px}"
         "h1{margin:4px 0;font-size:27px}.sub{color:#8b949e;margin-bottom:18px}"
         ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:11px}"
         ".card{background:#15191f;border:1px solid #2b3139;border-radius:14px;padding:15px;margin:12px 0}"
         "a.b,button{display:block;width:100%;padding:14px;border:1px solid #343b45;"
         "border-radius:11px;background:#20262e;color:#fff;text-decoration:none;text-align:center;"
         "font-size:15px}a.b:hover,button:hover{background:#2b333d}"
         "table{width:100%;border-collapse:collapse}th,td{padding:8px;text-align:left;"
         "border-bottom:1px solid #2a3038}th{color:#8b949e}.ok{color:#7ee787}"
         ".warn{color:#ffa657}.mono{font-family:monospace;word-break:break-all}"
         "input,select{width:100%;padding:11px;margin:5px 0 12px;background:#0d1117;color:#fff;"
         "border:1px solid #343b45;border-radius:8px}.tag{display:inline-block;padding:4px 8px;"
         "margin:3px;border-radius:20px;background:#252c35;color:#b9c1ca}.back{margin-top:14px}"
         "</style></head><body><div class='w'>");
  h += "<h1>ESP32-S3 CYBERDECK V2</h1><div class='sub'>" + esc(sub) + "</div>";
  return h;
}
String foot(){ return F("</div></body></html>"); }
void sendHTML(String h){ server.sendHeader("Cache-Control","no-store"); server.send(200,"text/html; charset=utf-8",h); }
String back(){ return F("<a class='b back' href='/'>Back</a>"); }

void root() {
  String h=head("Field toolbox / hardware console");
  h += F("<div class='grid'>"
         "<a class='b' href='/system'>System / Health</a>"
         "<a class='b' href='/wifi'>Wi-Fi Survey</a>"
         "<a class='b' href='/ble'>BLE Survey</a>"
         "<a class='b' href='/i2c'>I2C Scanner</a>"
         "<a class='b' href='/gpio'>GPIO Console</a>"
         "<a class='b' href='/adc'>ADC Meter</a>"
         "<a class='b' href='/pwm'>PWM Generator</a>"
         "<a class='b' href='/expansion'>Expansion Bay</a>"
         "</div>");
  h += "<div class='card'><b>Deck status</b><br>"
       "AP: <span class='mono'>"+String(AP_SSID)+"</span><br>"
       "IP: <span class='mono'>"+WiFi.softAPIP().toString()+"</span><br>"
       "Free heap: "+String(ESP.getFreeHeap())+" B<br>"
       "Free PSRAM: "+String(ESP.getFreePsram())+" B<br>"
       "Uptime: "+String((millis()-bootMs)/1000)+" s</div>";
  h += F("<div class='card'><span class='tag'>16 MB Flash</span>"
         "<span class='tag'>8 MB PSRAM</span><span class='tag'>Wi-Fi</span>"
         "<span class='tag'>BLE</span><span class='tag'>I2C</span>"
         "<span class='tag'>SPI</span><span class='tag'>ADC</span>"
         "<span class='tag'>PWM</span><span class='tag'>USB</span></div>");
  sendHTML(h+foot());
}

void systemPage() {
  esp_chip_info_t ci; esp_chip_info(&ci);
  String h=head("System and memory health");
  h += F("<div class='card'><table>");
  h += "<tr><th>Chip</th><td>ESP32-S3</td></tr>";
  h += "<tr><th>Cores</th><td>"+String(ci.cores)+"</td></tr>";
  h += "<tr><th>Revision</th><td>"+String(ci.revision)+"</td></tr>";
  h += "<tr><th>CPU</th><td>"+String(ESP.getCpuFreqMHz())+" MHz</td></tr>";
  h += "<tr><th>Flash</th><td>"+String(ESP.getFlashChipSize())+" bytes</td></tr>";
  h += "<tr><th>PSRAM</th><td>"+String(ESP.getPsramSize())+" bytes</td></tr>";
  h += "<tr><th>Free PSRAM</th><td>"+String(ESP.getFreePsram())+" bytes</td></tr>";
  h += "<tr><th>Heap</th><td>"+String(ESP.getHeapSize())+" bytes</td></tr>";
  h += "<tr><th>Free heap</th><td>"+String(ESP.getFreeHeap())+" bytes</td></tr>";
  h += "<tr><th>Min free heap</th><td>"+String(ESP.getMinFreeHeap())+" bytes</td></tr>";
  h += "<tr><th>SDK</th><td>"+String(ESP.getSdkVersion())+"</td></tr>";
  h += "<tr><th>Uptime</th><td>"+String((millis()-bootMs)/1000)+" seconds</td></tr>";
  h += F("</table></div>");
  sendHTML(h+back()+foot());
}

String authName(wifi_auth_mode_t a){
  switch(a){
    case WIFI_AUTH_OPEN:return "OPEN"; case WIFI_AUTH_WEP:return "WEP";
    case WIFI_AUTH_WPA_PSK:return "WPA"; case WIFI_AUTH_WPA2_PSK:return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/WPA3";
    default:return "Other";
  }
}

void wifiPage() {
  String h=head("Passive Wi-Fi environment survey");
  int n=WiFi.scanNetworks(false,true);
  h += "<div class='card'><b>"+String(n)+" networks found</b></div>";
  if(n>0){
    h += F("<div class='card'><table><tr><th>SSID</th><th>RSSI</th><th>CH</th><th>Security</th></tr>");
    for(int i=0;i<n;i++){
      h += "<tr><td>"+esc(WiFi.SSID(i))+"</td><td>"+String(WiFi.RSSI(i))+
           " dBm</td><td>"+String(WiFi.channel(i))+"</td><td>"+
           authName(WiFi.encryptionType(i))+"</td></tr>";
    }
    h += F("</table></div>");
  }
  WiFi.scanDelete();
  h += F("<a class='b' href='/wifi'>Rescan</a>");
  sendHTML(h+back()+foot());
}

void blePage() {
  String h=head("Passive Bluetooth LE advertisement survey");
  BLEScan *s=BLEDevice::getScan();
  s->setActiveScan(true); s->setInterval(100); s->setWindow(90);
  BLEScanResults r=s->start(5,false);
  int n=r.getCount();
  h += "<div class='card'><b>"+String(n)+" BLE devices found</b></div>";
  if(n){
    h += F("<div class='card'><table><tr><th>Name</th><th>Address</th><th>RSSI</th></tr>");
    for(int i=0;i<n;i++){
      BLEAdvertisedDevice d=r.getDevice(i);
      String nm=d.haveName()?String(d.getName().c_str()):"(unnamed)";
      h += "<tr><td>"+esc(nm)+"</td><td class='mono'>"+
           esc(String(d.getAddress().toString().c_str()))+"</td><td>"+
           String(d.getRSSI())+" dBm</td></tr>";
    }
    h += F("</table></div>");
  }
  s->clearResults();
  h += F("<a class='b' href='/ble'>Rescan</a>");
  sendHTML(h+back()+foot());
}

void i2cPage() {
  String h=head("I2C bus scanner");
  h += "<div class='card'>SDA GPIO "+String(I2C_SDA)+
       " &nbsp; SCL GPIO "+String(I2C_SCL)+"</div>";
  int found=0;
  h += F("<div class='card'><table><tr><th>Address</th><th>Status</th></tr>");
  for(uint8_t a=1;a<127;a++){
    Wire.beginTransmission(a);
    if(Wire.endTransmission()==0){
      char b[8]; snprintf(b,sizeof(b),"0x%02X",a);
      h += "<tr><td class='mono'>"+String(b)+"</td><td class='ok'>FOUND</td></tr>";
      found++;
    }
  }
  h += F("</table></div>");
  h += "<div class='card'>Devices found: "+String(found)+"</div>";
  h += F("<a class='b' href='/i2c'>Rescan</a>");
  sendHTML(h+back()+foot());
}

bool allowedGPIO(int p){
  // Conservative user-facing list; avoids USB pins 19/20 and flash/PSRAM-sensitive assumptions.
  const int pins[]={1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,18,21,35,36,37,38,39,40,41,42,47,48};
  for(int x:pins) if(x==p) return true;
  return false;
}

void gpioPage() {
  String msg="";
  if(server.hasArg("pin") && server.hasArg("action")){
    int p=server.arg("pin").toInt();
    if(allowedGPIO(p)){
      String a=server.arg("action");
      if(a=="high"){pinMode(p,OUTPUT);digitalWrite(p,HIGH);msg="GPIO "+String(p)+" = HIGH";}
      else if(a=="low"){pinMode(p,OUTPUT);digitalWrite(p,LOW);msg="GPIO "+String(p)+" = LOW";}
      else if(a=="input"){pinMode(p,INPUT);msg="GPIO "+String(p)+" set INPUT";}
    } else msg="Pin blocked by safe GPIO list.";
  }
  String h=head("GPIO console");
  h += F("<div class='card warn'>Connect only 3.3 V logic. Verify your board pinout before attaching hardware.</div>");
  if(msg.length()) h += "<div class='card'>"+esc(msg)+"</div>";
  h += F("<div class='card'><form method='get'><label>GPIO</label>"
         "<input name='pin' type='number' min='0' max='48' value='4'>"
         "<label>Action</label><select name='action'>"
         "<option value='high'>Output HIGH</option><option value='low'>Output LOW</option>"
         "<option value='input'>Input</option></select><button type='submit'>Apply</button></form></div>");
  sendHTML(h+back()+foot());
}

void adcPage() {
  int p=server.hasArg("pin")?server.arg("pin").toInt():4;
  String h=head("ADC meter");
  if(p<1 || p>10){
    h += F("<div class='card warn'>For this V2 tool choose ADC-capable GPIO 1-10.</div>");
  } else {
    int raw=analogRead(p);
    int mv=analogReadMilliVolts(p);
    h += "<div class='card'><b>GPIO "+String(p)+"</b><br>Raw: "+String(raw)+
         "<br>Approx: "+String(mv)+" mV</div>";
  }
  h += F("<div class='card'><form method='get'><label>ADC GPIO (1-10)</label>"
         "<input name='pin' type='number' min='1' max='10' value='4'>"
         "<button type='submit'>Read ADC</button></form></div>");
  sendHTML(h+back()+foot());
}

void pwmPage() {
  String msg="";
  if(server.hasArg("pin") && server.hasArg("freq") && server.hasArg("duty")){
    int p=server.arg("pin").toInt();
    int f=constrain(server.arg("freq").toInt(),1,20000);
    int duty=constrain(server.arg("duty").toInt(),0,255);
    if(allowedGPIO(p)){
      // Arduino-ESP32 2.x LEDC API used by current PlatformIO espressif32 package.
      const int ch=0;
      ledcSetup(ch,f,8);
      ledcAttachPin(p,ch);
      ledcWrite(ch,duty);
      msg="PWM active on GPIO "+String(p)+": "+String(f)+" Hz, duty "+String(duty)+"/255";
    } else msg="Pin blocked by safe GPIO list.";
  }
  String h=head("PWM / signal generator");
  h += F("<div class='card warn'>Logic-level signal generator only. Do not drive motors, relays or high-current loads directly from a GPIO.</div>");
  if(msg.length()) h += "<div class='card'>"+esc(msg)+"</div>";
  h += F("<div class='card'><form method='get'><label>GPIO</label>"
         "<input name='pin' type='number' value='4'><label>Frequency (1-20000 Hz)</label>"
         "<input name='freq' type='number' value='1000'><label>Duty (0-255)</label>"
         "<input name='duty' type='number' value='128'><button type='submit'>Start / Update PWM</button>"
         "</form></div>");
  sendHTML(h+back()+foot());
}

void expansionPage() {
  String h=head("Cyberdeck expansion bay");
  h += F("<div class='card'><b>Hardware-ready expansion plan</b><br><br>"
         "<span class='tag'>CC1101 sub-GHz</span>"
         "<span class='tag'>NRF24L01 2.4 GHz</span>"
         "<span class='tag'>IR TX/RX</span>"
         "<span class='tag'>125 kHz RFID</span>"
         "<span class='tag'>13.56 MHz NFC</span>"
         "<span class='tag'>microSD</span>"
         "<span class='tag'>GPS/GNSS</span>"
         "<span class='tag'>RTC</span>"
         "<span class='tag'>TFT Touch</span>"
         "<span class='tag'>Keyboard</span>"
         "<span class='tag'>Battery monitor</span>"
         "</div>"
         "<div class='card'>V2 keeps these modules disabled until their exact hardware and pins are configured. "
         "That prevents bus conflicts and makes this firmware usable on the bare S3 board now.</div>"
         "<div class='card'><b>Planned module capabilities</b><br>"
         "IR remote learning for your own equipment; NFC/RFID tag inspection; sub-GHz spectrum/RSSI tools; "
         "2.4 GHz packet-development tools; GPS logging; SD file manager; serial/UART console; "
         "touch UI; environmental sensors; battery/power dashboard.</div>");
  sendHTML(h+back()+foot());
}

void setup(){
  bootMs=millis();
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32-S3 CYBERDECK TOOLBOX V2");
  Serial.printf("Flash: %u\n",ESP.getFlashChipSize());
  Serial.printf("PSRAM: %u\n",ESP.getPsramSize());

  Wire.begin(I2C_SDA,I2C_SCL);
  analogReadResolution(12);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID,AP_PASS);
  BLEDevice::init("S3-Cyberdeck");

  server.on("/",root);
  server.on("/system",systemPage);
  server.on("/wifi",wifiPage);
  server.on("/ble",blePage);
  server.on("/i2c",i2cPage);
  server.on("/gpio",gpioPage);
  server.on("/adc",adcPage);
  server.on("/pwm",pwmPage);
  server.on("/expansion",expansionPage);
  server.onNotFound([](){server.send(404,"text/plain","404");});
  server.begin();

  Serial.printf("AP: %s\n",AP_SSID);
  Serial.printf("Password: %s\n",AP_PASS);
  Serial.print("Open: http://"); Serial.println(WiFi.softAPIP());
}
void loop(){server.handleClient();delay(2);}
