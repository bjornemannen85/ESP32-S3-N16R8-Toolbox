#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// V4.4.2 shared SPI expansion bus.
// -1 means deliberately unassigned until final display/board pinout is confirmed.
static int V42_SPI_SCK=-1, V42_SPI_MISO=-1, V42_SPI_MOSI=-1;
static int V42_SD_CS=-1;
static int V42_CC1101_CS=-1, V42_CC1101_GDO0=-1, V42_CC1101_GDO2=-1;
bool v42SpiOK(){return V42_SPI_SCK>=0&&V42_SPI_MISO>=0&&V42_SPI_MOSI>=0;}
bool v42SdOK(){return v42SpiOK()&&V42_SD_CS>=0;}
bool v42CcOK(){return v42SpiOK()&&V42_CC1101_CS>=0;}

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <Preferences.h>

// ESP32-S3 N16R8 CYBERDECK TOOLBOX V4.4.2
// Adds local Wi-Fi setup without storing home credentials in GitHub/source.
// The fallback AP remains available for configuration and recovery.


WebServer server(80);

// V4.4.2.1 UART Lab - RX/TX diagnostic interface.
// Generic serial TX is available for equipment you own/are authorized to service.
HardwareSerial AnalyzerUART(1);
bool uartAnalyzerRunning = false;
int uartAnalyzerRxPin = -1;
int uartAnalyzerTxPin = -1;
uint32_t uartAnalyzerBaud = 4800;
String uartCapture = "";
const size_t UART_CAPTURE_MAX = 8192;

bool uartAnalyzerPinAllowed(int pin) {
  // Keep native USB and the shared I2C bus protected.
  if (pin < 0 || pin > 48) return false;
  if (pin == 8 || pin == 9 || pin == 19 || pin == 20) return false;
  return true;
}

bool startUARTAnalyzer(int rxPin, int txPin, uint32_t baud) {
  if (!uartAnalyzerPinAllowed(rxPin) || !uartAnalyzerPinAllowed(txPin) || rxPin == txPin) return false;
  if (!(baud==1200 || baud==2400 || baud==4800 || baud==9600 ||
        baud==19200 || baud==38400 || baud==57600 || baud==115200)) return false;

  if (uartAnalyzerRunning) AnalyzerUART.end();
  AnalyzerUART.begin(baud, SERIAL_8N1, rxPin, txPin);
  uartAnalyzerRxPin = rxPin;
  uartAnalyzerTxPin = txPin;
  uartAnalyzerBaud = baud;
  uartAnalyzerRunning = true;

  Preferences u;
  if (u.begin("uartcfg", false)) {
    u.putInt("rx", rxPin);
    u.putInt("tx", txPin);
    u.putULong("baud", baud);
    u.end();
  }
  return true;
}


bool sendTimedUARTPacket(uint8_t command,
                         uint8_t a, uint8_t b, uint8_t c,
                         uint8_t d, uint8_t e) {
  if (!uartAnalyzerRunning || uartAnalyzerTxPin < 0)
    return false;

  uint8_t checksum =
      (uint8_t)(command + a + b + c + d + e);

  // Temporarily release UART so TX GPIO can generate
  // the required LOW/HIGH synchronization pulse.
  AnalyzerUART.flush();
  AnalyzerUART.end();

  pinMode(uartAnalyzerTxPin, OUTPUT);
  digitalWrite(uartAnalyzerTxPin, LOW);
  delayMicroseconds(2750);

  digitalWrite(uartAnalyzerTxPin, HIGH);
  delayMicroseconds(200);

  // Restore UART at 4800 baud / 8N1.
  AnalyzerUART.begin(
      4800,
      SERIAL_8N1,
      uartAnalyzerRxPin,
      uartAnalyzerTxPin
  );

  AnalyzerUART.write((uint8_t)0x00);
  AnalyzerUART.write(command);
  AnalyzerUART.write(a);
  AnalyzerUART.write(b);
  AnalyzerUART.write(c);
  AnalyzerUART.write(d);
  AnalyzerUART.write(e);
  AnalyzerUART.write(checksum);

  AnalyzerUART.flush();

  logEvent(
      "Timed UART packet sent: command=0x" +
      String(command, HEX) +
      " checksum=0x" +
      String(checksum, HEX)
  );

  return true;
}

bool uartSendHex(String input) {
  if (!uartAnalyzerRunning) return false;
  input.replace(" ", "");
  input.replace(":", "");
  input.replace("-", "");
  if (input.length()==0 || (input.length() & 1)) return false;
  for (size_t i=0; i<input.length(); i+=2) {
    char a=input[i], b=input[i+1];
    if (!isxdigit((unsigned char)a) || !isxdigit((unsigned char)b)) return false;
  }
  for (size_t i=0; i<input.length(); i+=2) {
    char tmp[3] = { input[i], input[i+1], 0 };
    AnalyzerUART.write((uint8_t)strtoul(tmp, nullptr, 16));
  }
  AnalyzerUART.flush();
  return true;
}

bool uartSendText(const String &data) {
  if (!uartAnalyzerRunning || data.length()==0) return false;
  AnalyzerUART.print(data);
  AnalyzerUART.flush();
  return true;
}

void stopUARTAnalyzer() {
  if (uartAnalyzerRunning) AnalyzerUART.end();
  uartAnalyzerRunning = false;
}

void pollUARTAnalyzer() {
  if (!uartAnalyzerRunning) return;
  while (AnalyzerUART.available()) {
    uint8_t b = (uint8_t)AnalyzerUART.read();
    char h[4];
    snprintf(h, sizeof(h), "%02X ", b);
    uartCapture += h;
    if (uartCapture.length() > UART_CAPTURE_MAX)
      uartCapture.remove(0, uartCapture.length() - UART_CAPTURE_MAX);
  }
}

String uartAnalyzerPage() {
  String h = "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<meta http-equiv='refresh' content='3'></head><body>"
             "<h2>V4.4.2 UART Lab</h2>"
             "<p>Generic RX/TX serial diagnostic interface.</p>";
  h += "<p>Status: " + String(uartAnalyzerRunning ? "RUNNING" : "STOPPED") + "</p>";
  h += "<p>RX GPIO: " + String(uartAnalyzerRxPin) + " | TX GPIO: " + String(uartAnalyzerTxPin) +
       " | Baud: " + String(uartAnalyzerBaud) + "</p>";
  h += "<form action='/uart/start' method='get'>RX: <input name='rx' type='number' size='3'> "
       "TX: <input name='tx' type='number' size='3'> Baud: <select name='baud'>"
       "<option>1200</option><option>2400</option><option selected>4800</option>"
       "<option>9600</option><option>19200</option><option>38400</option>"
       "<option>57600</option><option>115200</option></select> <button>Start</button></form>";
  h += "<p><a href='/uart/stop'>Stop</a> | <a href='/uart/clear'>Clear capture</a></p>";
  h += "<h3>Transmit text</h3><form action='/uart/sendtext' method='post'>"
       "<input name='data' style='width:70%'><button>Send</button></form>";
  h += "<h3>Transmit HEX</h3><form action='/uart/sendhex' method='post'>"
       "<input name='data' placeholder='48 65 6C 6C 6F' style='width:70%'><button>Send HEX</button></form>";
  h += "<h3>RX HEX capture</h3><pre style='white-space:pre-wrap'>" + uartCapture + "</pre>";
  h += "<p><a href='/'>Back</a></p></body></html>";
  return h;
}


static const char *AP_SSID = "S3-Cyberdeck";
static const char *AP_PASS = "cyberdeck123";
static const char *SETUP_PATH = "/network";
Preferences prefs;
String staSsid;

bool staConnected = false;

// ---------- V4 software core ----------
struct PinClaim { int gpio; const char* owner; bool reserved; };
PinClaim pinClaims[] = {
  {19,"Native USB D-",true},{20,"Native USB D+",true},
  {8,"I2C SDA",true},{9,"I2C SCL",true}
};

struct ModuleState { const char* id; const char* name; bool enabled; bool present; };
ModuleState modules[] = {
  {"display","ILI9488 Display",false,false},{"touch","SPI Touch",false,false},
  {"cc1101","CC1101 Sub-GHz",false,false},{"pn532","PN532 NFC",false,false},
  {"nrf24","nRF24L01+",false,false},{"ir","IR TX/RX",false,false},
  {"ina219","INA219 Power Monitor",false,false},{"rtc","DS3231 RTC",false,false},
  {"sd","microSD",false,false}
};

const int LOG_MAX=40;
String eventLog[LOG_MAX];
int eventHead=0,eventCount=0;

void logEvent(const String& msg){
  eventLog[eventHead]="["+String(millis()/1000)+"s] "+msg;
  eventHead=(eventHead+1)%LOG_MAX;
  if(eventCount<LOG_MAX) eventCount++;
  Serial.println("[V4] "+msg);
}

String moduleSummary(){
  String r;
  for(auto &m:modules) r+=String(m.name)+": "+(m.enabled?"ENABLED":"disabled")+" / "+(m.present?"ONLINE":"offline")+"\n";
  return r;
}
String pinSummary(){
  String r;
  for(auto &p:pinClaims) r+="GPIO"+String(p.gpio)+" -> "+p.owner+(p.reserved?" [RESERVED]":"")+"\n";
  return r;
}

// Forward declarations for V4.4.2
String v42SpiInfo();
bool v41ReservedPin(int pin);
bool loadSavedNetwork(String &ssid, String &pass);
bool saveNetworkPermanent(const String &ssid, const String &pass);

String runAdminCommand(String cmd){
  cmd.trim(); String lc=cmd; lc.toLowerCase();
  if(lc=="help") return "Commands: help, status, wifi, wifi saved, uart status, uart start <rx> <tx> <baud>, uart stop, uart tx <text>, uart txhex <hex>, uart rx, uart clear, scan wifi, scan ble, i2c scan, i2c modules, storage, spi, cc1101, log status, modules, pins, logs, heap, psram, uptime, gpio read <pin>, adc read <pin>, reboot";
  if(lc=="uart status"){
    return "UART: "+String(uartAnalyzerRunning?"RUNNING":"STOPPED")+
           " | RX="+String(uartAnalyzerRxPin)+
           " | TX="+String(uartAnalyzerTxPin)+
           " | baud="+String(uartAnalyzerBaud);
  }
  if(lc.startsWith("uart start ")){
    int rx=-1, tx=-1;
    unsigned long baud=0;
    if(sscanf(cmd.c_str(), "uart start %d %d %lu", &rx, &tx, &baud)==3) {
      return startUARTAnalyzer(rx, tx, baud)
        ? "UART started: RX="+String(rx)+" TX="+String(tx)+" baud="+String(baud)
        : "ERROR: invalid/reserved pins or unsupported baud";
    }
    return "Usage: uart start <rx> <tx> <baud>";
  }
  if(lc=="uart stop"){
    stopUARTAnalyzer();
    return "UART stopped";
  }
  if(lc=="uart clear"){
    uartCapture="";
    return "UART capture cleared";
  }
  if(lc.startsWith("uart txhex ")){
    String data=cmd.substring(11);
    data.trim();
    return uartSendHex(data) ? "UART HEX sent" : "ERROR: UART not running or invalid HEX";
  }
  if(lc.startsWith("uart tx ")){
    String data=cmd.substring(8);
    return uartSendText(data) ? "UART text sent" : "ERROR: UART not running or empty data";
  }
  if(lc=="uart rx"){
    if(!uartCapture.length()) return "UART RX capture is empty";
    String tail=uartCapture;
    if(tail.length()>1200) tail=tail.substring(tail.length()-1200);
    return tail;
  }
  if(lc=="status") return "Cyberdeck V4\nWiFi: "+String(WiFi.status()==WL_CONNECTED?"CONNECTED":"offline")+
    "\nLAN IP: "+WiFi.localIP().toString()+"\nAP IP: "+WiFi.softAPIP().toString()+
    "\nHeap: "+String(ESP.getFreeHeap())+"\nPSRAM: "+String(ESP.getFreePsram());
  if(lc=="wifi saved"){
    String ss, pw; bool ok=loadSavedNetwork(ss,pw);
    return ok ? "Saved WiFi in NVS: "+ss+" | password stored: "+String(pw.length()?"yes":"no")
              : "No saved WiFi credentials in NVS";
  }
  if(lc=="wifi") return "STA: "+String(WiFi.status()==WL_CONNECTED?WiFi.SSID():"not connected")+
    "\nLAN IP: "+WiFi.localIP().toString()+"\nRSSI: "+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+" dBm";
  if(lc=="spi") return v42SpiInfo();
  if(lc=="storage") return v42SdOK() ? "microSD SPI pins assigned" : "microSD pins pending final hardware pinout";
  if(lc=="log status") return v42SdOK() ? "SD logging bus configured" : "SD logging pending final SPI/CS pinout";
  if(lc=="cc1101") return v42CcOK() ? "CC1101 SPI pins assigned; init pending module verification" : "CC1101 pins pending final hardware pinout";
  if(lc=="scan wifi"){
    int n=WiFi.scanNetworks(false,true); String r="WiFi: "+String(n)+" network(s)\n";
    for(int i=0;i<n;i++)r+=WiFi.SSID(i)+" | "+String(WiFi.RSSI(i))+" dBm | ch "+String(WiFi.channel(i))+"\n";
    WiFi.scanDelete(); return r;
  }
  if(lc=="scan ble"){
    BLEScan* sc=BLEDevice::getScan(); sc->setActiveScan(false); BLEScanResults rr=sc->start(4,false);
    String r="BLE: "+String(rr.getCount())+" device(s)\n";
    for(int i=0;i<rr.getCount();i++){BLEAdvertisedDevice d=rr.getDevice(i);r+=String(d.getAddress().toString().c_str())+" | "+String(d.getRSSI())+" dBm\n";}
    sc->clearResults(); return r;
  }
  if(lc=="i2c modules"){
    String r="I2C module inventory\n"; int n=0;
    for(uint8_t a=1;a<127;a++){
      Wire.beginTransmission(a);
      if(Wire.endTransmission()==0){
        n++; r+="0x"; if(a<16) r+="0"; r+=String(a,HEX);
        if(a==0x68) r+=" | RTC candidate (DS3231)";
        else if(a==0x40 || a==0x41 || a==0x44 || a==0x45) r+=" | INA219/INA2xx candidate";
        else if(a>=0x50 && a<=0x57) r+=" | EEPROM candidate";
        r+="\n";
      }
    }
    return "Detected "+String(n)+" I2C device(s)\n"+r;
  }
  if(lc=="i2c scan"){
    String r; int n=0; for(uint8_t a=1;a<127;a++){Wire.beginTransmission(a);if(Wire.endTransmission()==0){n++;r+="0x";if(a<16)r+="0";r+=String(a,HEX)+"\n";}}
    return "I2C: "+String(n)+" device(s)\n"+r;
  }
  if(lc=="modules") return moduleSummary();
  if(lc=="pins") return pinSummary();
  if(lc=="heap") return String(ESP.getFreeHeap())+" bytes";
  if(lc=="psram") return String(ESP.getFreePsram())+" bytes";
  if(lc=="uptime") return String(millis()/1000)+" seconds";
  if(lc=="logs"){
    String r;
    for(int i=0;i<eventCount;i++){ int x=(eventHead-eventCount+i+LOG_MAX)%LOG_MAX; r+=eventLog[x]+"\n"; }
    return r.length()?r:"No events";
  }
  if(lc.startsWith("gpio read ")){
    int pin=lc.substring(10).toInt(); if(pin<0||pin>48) return "Invalid GPIO"; if(v41ReservedPin(pin)) return "GPIO reserved by Pin Manager";
    pinMode(pin,INPUT); return "GPIO"+String(pin)+"="+String(digitalRead(pin));
  }
  if(lc.startsWith("adc read ")){
    int pin=lc.substring(9).toInt(); if(pin<0||pin>20) return "Invalid ADC GPIO";
    return "ADC GPIO"+String(pin)+"="+String(analogRead(pin));
  }
  if(lc=="reboot"){ logEvent("Admin requested reboot"); return "REBOOT_PENDING"; }
  return "Unknown command. Type: help";
}


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
  h += "<h1>ESP32-S3 CYBERDECK V4.4.2</h1><div class='sub'>" + esc(sub) + "</div>";
  return h;
}
String foot(){ return F("</div></body></html>"); }
void sendHTML(String h){ server.sendHeader("Cache-Control","no-store"); server.send(200,"text/html; charset=utf-8",h); }
String back(){ return F("<a class='b back' href='/'>Back</a>"); }

void root() {
  String h=head("Field toolbox / hardware console");
  h += F("<div class='grid'>"
         "<a class='b' href='/system'>System / Health</a>"
         "<a class='b' href='/v43-modules'>V4.4.2 Module Status</a>"
         "<a class='b' href='/storage'>Storage Manager</a>"
         "<a class='b' href='/cc1101'>CC1101 RF</a>"
         "<a class='b' href='/wifi-analyzer'>WiFi Analyzer</a>"
         "<a class='b' href='/ble-explorer'>BLE Explorer</a>"
         "<a class='b' href='/i2c-explorer'>I2C Explorer</a>"
         "<a class='b' href='/gpio-lab'>GPIO Lab</a>"
         "<a class='b' href='/monitor'>System Monitor</a>"
         "<a class='b' href='/terminal'>Admin Terminal</a>"
         "<a class='b' href='/uart'>UART Lab</a>"
         "<a class='b' href='/diagnostics'>Diagnostics</a>"
         "<a class='b' href='/modules'>Module Manager</a>"
         "<a class='b' href='/pins'>Pin Manager</a>"
         "<a class='b' href='/logs'>Event Log</a>"
         "<a class='b' href='/network'>Network Setup</a>"
         "<a class='b' href='/wifi'>Wi-Fi Survey</a>"
         "<a class='b' href='/ble'>BLE Survey</a>"
         "<a class='b' href='/i2c'>I2C Scanner</a>"
         "<a class='b' href='/gpio'>GPIO Console</a>"
         "<a class='b' href='/adc'>ADC Meter</a>"
         "<a class='b' href='/pwm'>PWM Generator</a>"
         "<a class='b' href='/expansion'>Expansion Bay</a>"
         "</div>");
  h += "<div class='card'><b>Deck status</b><br>"
       "Fallback AP: <span class='mono'>"+String(AP_SSID)+"</span><br>"
       "AP IP: <span class='mono'>"+WiFi.softAPIP().toString()+"</span><br>";
  if(WiFi.status()==WL_CONNECTED){
    h += "Home Wi-Fi: <span class='ok'>CONNECTED</span><br>"
         "SSID: <span class='mono'>"+esc(WiFi.SSID())+"</span><br>"
         "LAN IP: <span class='mono'>"+WiFi.localIP().toString()+"</span><br>";
  } else {
    h += "Home Wi-Fi: <span class='warn'>NOT CONNECTED</span><br>"
         "<a href='/network'>Configure network</a><br>";
  }
  h += "Free heap: "+String(ESP.getFreeHeap())+" B<br>"
       "Free PSRAM: "+String(ESP.getFreePsram())+" B<br>"
       "Uptime: "+String((millis()-bootMs)/1000)+" s</div>";
  h += F("<div class='card'><span class='tag'>16 MB Flash</span>"
         "<span class='tag'>8 MB PSRAM</span><span class='tag'>Wi-Fi</span>"
         "<span class='tag'>BLE</span><span class='tag'>I2C</span>"
         "<span class='tag'>SPI</span><span class='tag'>ADC</span>"
         "<span class='tag'>PWM</span><span class='tag'>USB</span></div>");
  sendHTML(h+foot());
}





String v42SpiInfo(){
  if(!v42SpiOK()) return "SPI pins pending final hardware pinout";
  return "SCK GPIO"+String(V42_SPI_SCK)+", MISO GPIO"+String(V42_SPI_MISO)+", MOSI GPIO"+String(V42_SPI_MOSI);
}
void storagePage(){
  String h=head("Storage Manager");
  h+="<div class='card'><b>Shared SPI</b><br>"+esc(v42SpiInfo())+"<br>SD CS: "+String(V42_SD_CS)+"</div>";
  if(!v42SdOK()) h+=F("<div class='card'>SD driver ready. GPIO assignment is intentionally disabled until the final board/display pinout is confirmed.</div>");
  else {
    SPI.begin(V42_SPI_SCK,V42_SPI_MISO,V42_SPI_MOSI,V42_SD_CS);
    if(SD.begin(V42_SD_CS,SPI)){
      h+="<div class='card'>microSD mounted<br>Card: "+String((uint32_t)(SD.cardSize()/1048576ULL))+" MB"+
         "<br>Used: "+String((uint32_t)(SD.usedBytes()/1048576ULL))+" MB</div>";
      SD.end();
    } else h+=F("<div class='card'>microSD mount failed.</div>");
  }
  sendHTML(h+back()+foot());
}

void v43ModulesPage(){
  String h=head("V4.4.2 Module Status");
  h+=F("<div class='card'><b>I2C bus</b><br>SDA GPIO8 / SCL GPIO9<br>Prepared: DS3231, INA219 and PN532 integration.</div>");
  h+="<div class='card'><b>SPI bus</b><br>"+esc(v42SpiInfo())+
     "<br>Prepared: CC1101 + microSD with independent CS pins.</div>";
  h+=F("<div class='card'><b>Safety</b><br>Hardware-dependent SPI/CC1101 pins remain disabled until the final board/display pinout is confirmed.</div>");
  sendHTML(h+back()+foot());
}

void cc1101Page(){
  String h=head("CC1101 RF");
  h+=F("<div class='card'>CC1101 expansion slot prepared for passive Sub-GHz diagnostics, RSSI measurements, module detection and SD signal logging.</div>");
  h+="<div class='card'>"+esc(v42SpiInfo())+"<br>CS: "+String(V42_CC1101_CS)+
     "<br>GDO0: "+String(V42_CC1101_GDO0)+"<br>GDO2: "+String(V42_CC1101_GDO2)+"</div>";
  if(!v42CcOK()) h+=F("<div class='card'>GPIO assignment pending final hardware pinout.</div>");
  else h+=F("<div class='card'>SPI assignment ready. Radio initialization remains disabled until the exact module/frequency variant is verified.</div>");
  sendHTML(h+back()+foot());
}

bool v41ReservedPin(int pin){
  for(auto &p:pinClaims) if(p.gpio==pin && p.reserved) return true;
  return false;
}
String wifiSec(wifi_auth_mode_t a){
  if(a==WIFI_AUTH_OPEN)return "OPEN"; if(a==WIFI_AUTH_WEP)return "WEP";
  if(a==WIFI_AUTH_WPA_PSK)return "WPA"; if(a==WIFI_AUTH_WPA2_PSK)return "WPA2";
  if(a==WIFI_AUTH_WPA_WPA2_PSK)return "WPA/WPA2";
  if(a==WIFI_AUTH_WPA3_PSK)return "WPA3"; if(a==WIFI_AUTH_WPA2_WPA3_PSK)return "WPA2/WPA3";
  return "OTHER";
}
void wifiAnalyzerPage(){
  int n=WiFi.scanNetworks(false,true); String h=head("WiFi Analyzer");
  h+=F("<div class='card'>Passive survey only.</div>");
  for(int i=0;i<n;i++) h+="<div class='card'><b>"+esc(WiFi.SSID(i))+"</b><br>RSSI: "+String(WiFi.RSSI(i))+
    " dBm<br>Channel: "+String(WiFi.channel(i))+"<br>Security: "+wifiSec(WiFi.encryptionType(i))+
    "<br>BSSID: <span class='mono'>"+WiFi.BSSIDstr(i)+"</span></div>";
  if(n<=0) h+=F("<div class='card'>No networks found.</div>");
  WiFi.scanDelete(); sendHTML(h+back()+foot());
}
void bleExplorerPage(){
  BLEScan* sc=BLEDevice::getScan(); sc->setActiveScan(false); BLEScanResults rr=sc->start(4,false);
  String h=head("BLE Explorer"); h+=F("<div class='card'>Passive BLE advertisement scan.</div>");
  for(int i=0;i<rr.getCount();i++){ BLEAdvertisedDevice d=rr.getDevice(i);
    String nm=d.haveName()?String(d.getName().c_str()):"(unnamed)";
    h+="<div class='card'><b>"+esc(nm)+"</b><br>Address: <span class='mono'>"+
       String(d.getAddress().toString().c_str())+"</span><br>RSSI: "+String(d.getRSSI())+" dBm</div>";
  }
  sc->clearResults(); sendHTML(h+back()+foot());
}
String knownI2C(uint8_t a){
  if(a==0x40||a==0x41||a==0x44||a==0x45)return "INA219/INA2xx candidate";
  if(a==0x68)return "DS3231/RTC candidate";
  if(a>=0x50&&a<=0x57)return "EEPROM candidate";
  return "";
}
void i2cExplorerPage(){
  String h=head("I2C Explorer"); h+=F("<div class='card'>SDA GPIO8 / SCL GPIO9</div>"); int n=0;
  for(uint8_t a=1;a<127;a++){ Wire.beginTransmission(a); if(Wire.endTransmission()==0){ n++;
    String x=knownI2C(a); h+="<div class='card'><b>0x"; if(a<16)h+="0"; h+=String(a,HEX)+"</b>";
    if(x.length())h+="<br>"+x; h+="</div>"; }}
  if(!n)h+=F("<div class='card'>No I2C devices detected.</div>"); sendHTML(h+back()+foot());
}
void gpioLabPage(){
  String r;
  if(server.hasArg("pin")&&server.hasArg("op")){ int pin=server.arg("pin").toInt(); String op=server.arg("op");
    if(pin<0||pin>48)r="Invalid GPIO";
    else if(v41ReservedPin(pin))r="Blocked: GPIO"+String(pin)+" is reserved";
    else if(op=="read"){pinMode(pin,INPUT);r="GPIO"+String(pin)+"="+String(digitalRead(pin));}
    else if(op=="adc")r="ADC GPIO"+String(pin)+"="+String(analogRead(pin));
  }
  String h=head("GPIO Lab");
  h+=F("<div class='card'>Read-only by default. Reserved pins are blocked.</div><div class='card'><form method='get' action='/gpio-lab'><input name='pin' inputmode='numeric' placeholder='GPIO'><select name='op'><option value='read'>Digital read</option><option value='adc'>ADC read</option></select><button type='submit'>Run</button></form></div>");
  if(r.length())h+="<div class='card mono'>"+esc(r)+"</div>"; sendHTML(h+back()+foot());
}
void monitorPage(){
  String h=head("System Monitor");
  h+="<div class='card'>Uptime: "+String(millis()/1000)+" s<br>CPU: "+String(ESP.getCpuFreqMHz())+
     " MHz<br>Free heap: "+String(ESP.getFreeHeap())+" B<br>Free PSRAM: "+String(ESP.getFreePsram())+
     " B<br>WiFi RSSI: "+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+" dBm</div>";
  h+=F("<div class='card'><a class='b' href='/monitor'>Refresh</a></div>"); sendHTML(h+back()+foot());
}

void terminalPage(){
  String output,cmd;
  if(server.method()==HTTP_POST && server.hasArg("cmd")){
    cmd=server.arg("cmd"); output=runAdminCommand(cmd); logEvent("Terminal command: "+cmd);
  }
  String h=head("Admin Terminal");
  h+=F("<div class='card'><b>Firmware admin console</b><br>This is not Linux root. Type <span class='mono'>help</span>.</div>"
       "<div class='card'><form method='post' action='/terminal'><input name='cmd' class='mono' autocomplete='off' placeholder='help'><button type='submit'>Run</button></form></div>");
  if(output.length()){ String s=esc(output); s.replace("\n","<br>"); h+="<div class='card mono'>"+s+"</div>"; }
  sendHTML(h+back()+foot());
  if(output=="REBOOT_PENDING"){ delay(750); ESP.restart(); }
}

void diagnosticsPage(){
  String h=head("Diagnostics");
  h+="<div class='card'><b>CPU</b><br>Chip: "+String(ESP.getChipModel())+"<br>Cores: "+String(ESP.getChipCores())+"<br>CPU: "+String(ESP.getCpuFreqMHz())+" MHz</div>";
  h+="<div class='card'><b>Memory</b><br>Free heap: "+String(ESP.getFreeHeap())+" B<br>Free PSRAM: "+String(ESP.getFreePsram())+" B<br>Flash: "+String(ESP.getFlashChipSize())+" B</div>";
  h+="<div class='card'><b>Network</b><br>STA: "+String(WiFi.status()==WL_CONNECTED?"CONNECTED":"offline")+"<br>LAN IP: "+WiFi.localIP().toString()+"<br>AP IP: "+WiFi.softAPIP().toString()+"<br>RSSI: "+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+" dBm</div>";
  sendHTML(h+back()+foot());
}

void modulesPage(){
  if(server.hasArg("toggle")){
    String id=server.arg("toggle");
    for(auto &m:modules) if(id==m.id){
      m.enabled=!m.enabled; prefs.begin("v4mods",false); prefs.putBool(m.id,m.enabled); prefs.end();
      logEvent(String(m.name)+(m.enabled?" enabled":" disabled"));
    }
  }
  String h=head("Module Manager");
  h+=F("<div class='card'>Module switches are prepared now; hardware detection activates when modules are connected.</div>");
  for(auto &m:modules) h+="<div class='card'><b>"+String(m.name)+"</b><br>Software: "+String(m.enabled?"<span class='ok'>ENABLED</span>":"disabled")+"<br>Hardware: "+String(m.present?"<span class='ok'>ONLINE</span>":"offline")+"<br><a class='b' href='/modules?toggle="+String(m.id)+"'>Toggle</a></div>";
  sendHTML(h+back()+foot());
}

void pinsPage(){
  String h=head("Pin Manager");
  h+=F("<div class='card'>Reserved pins prevent accidental hardware conflicts.</div>");
  for(auto &p:pinClaims) h+="<div class='card'><b>GPIO"+String(p.gpio)+"</b> &rarr; "+String(p.owner)+(p.reserved?" <span class='warn'>RESERVED</span>":"")+"</div>";
  sendHTML(h+back()+foot());
}

void logsPage(){
  String h=head("Event Log"); h+="<div class='card mono'>";
  if(!eventCount) h+="No events yet.";
  for(int i=0;i<eventCount;i++){ int x=(eventHead-eventCount+i+LOG_MAX)%LOG_MAX; h+=esc(eventLog[x])+"<br>"; }
  h+="</div>"; sendHTML(h+back()+foot());
}

void loadV4Settings(){
  prefs.begin("v4mods",true);
  for(auto &m:modules) m.enabled=prefs.getBool(m.id,false);
  prefs.end();
}

bool loadSavedNetwork(String &ssid, String &pass) {
  Preferences net;
  if(!net.begin("cybernet", true)) return false;
  ssid = net.getString("ssid", "");
  pass = net.getString("pass", "");
  uint32_t magic = net.getUInt("magic", 0);
  net.end();

  // One-time migration from the older V3/V4 namespace.
  if(ssid.length()==0 || magic!=0x43594252UL) {
    Preferences legacy;
    if(legacy.begin("net", true)) {
      String oldSsid = legacy.getString("ssid", "");
      String oldPass = legacy.getString("pass", "");
      legacy.end();
      if(oldSsid.length()) {
        Preferences dst;
        if(dst.begin("cybernet", false)) {
          dst.putString("ssid", oldSsid);
          dst.putString("pass", oldPass);
          dst.putUInt("magic", 0x43594252UL);
          dst.end();
          ssid=oldSsid; pass=oldPass;
        }
      }
    }
  }
  return ssid.length()>0;
}

bool saveNetworkPermanent(const String &ssid, const String &pass) {
  Preferences net;
  if(!net.begin("cybernet", false)) return false;
  size_t a=net.putString("ssid", ssid);
  size_t b=net.putString("pass", pass);
  size_t c=net.putUInt("magic", 0x43594252UL);
  net.end();

  String verifySsid, verifyPass;
  bool ok=loadSavedNetwork(verifySsid, verifyPass);
  return ok && a>0 && b>0 && c>0 && verifySsid==ssid && verifyPass==pass;
}

void connectSavedWiFi() {
  String pass;
  if(!loadSavedNetwork(staSsid, pass)) {
    Serial.println("No saved home Wi-Fi in NVS.");
    return;
  }

  Serial.printf("Connecting to Wi-Fi: %s\n", staSsid.c_str());
  WiFi.begin(staSsid.c_str(), pass.c_str());

  unsigned long start = millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start < 15000) {
    delay(250);
  }
  staConnected = (WiFi.status()==WL_CONNECTED);
  if(staConnected) {
    Serial.print("LAN IP: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Saved Wi-Fi unavailable; fallback AP remains active.");
  }
}

void networkPage() {
  String msg;

  if(server.method()==HTTP_POST && server.hasArg("ssid")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    ssid.trim();

    if(ssid.length()) {
      bool savedOK = saveNetworkPermanent(ssid, pass);
      if(!savedOK) {
        msg = "ERROR: Could not verify Wi-Fi credentials in NVS.";
        logEvent("WiFi NVS save verification FAILED");
      } else {
        logEvent("WiFi credentials saved and verified in NVS");
      }

      WiFi.disconnect(false, false);
      delay(200);
      WiFi.begin(ssid.c_str(), pass.c_str());

      unsigned long start = millis();
      while(WiFi.status()!=WL_CONNECTED && millis()-start < 15000) delay(250);

      if(WiFi.status()==WL_CONNECTED) {
        staSsid = ssid;
        staConnected = true;
        msg = "Connected. LAN IP: " + WiFi.localIP().toString();
      } else {
        staConnected = false;
        msg = "Credentials saved, but connection failed. Check password/signal and try again.";
      }
    }
  }

  if(server.hasArg("forget") && server.arg("forget")=="1") {
    Preferences net;
    if(net.begin("cybernet", false)) { net.clear(); net.end(); }
    Preferences legacy;
    if(legacy.begin("net", false)) { legacy.clear(); legacy.end(); }
    WiFi.disconnect(true, false);
    staSsid = "";
    staConnected = false;
    msg = "Saved home Wi-Fi credentials cleared.";
  }

  String h=head("Local network setup");
  h += F("<div class='card'>The Cyberdeck fallback AP stays enabled. Home Wi-Fi credentials are stored only in ESP32 NVS, not in the source code or GitHub.</div>");

  if(msg.length()) h += "<div class='card'>"+esc(msg)+"</div>";

  h += "<div class='card'><b>Status</b><br>";
  if(WiFi.status()==WL_CONNECTED) {
    h += "Connected to: <span class='mono'>"+esc(WiFi.SSID())+"</span><br>"
         "LAN IP: <span class='mono'>"+WiFi.localIP().toString()+"</span><br>"
         "RSSI: "+String(WiFi.RSSI())+" dBm";
  } else {
    h += "<span class='warn'>Not connected to home Wi-Fi</span>";
  }
  h += "</div>";

  int n=WiFi.scanNetworks(false,true);
  h += F("<div class='card'><form method='post' action='/network'>"
         "<label>Wi-Fi network</label><select name='ssid'>");
  if(n>0) {
    for(int i=0;i<n;i++) {
      String s=WiFi.SSID(i);
      if(!s.length()) continue;
      h += "<option value='"+esc(s)+"'>"+esc(s)+" ("+String(WiFi.RSSI(i))+" dBm)</option>";
    }
  }
  h += F("</select><label>Wi-Fi password</label>"
         "<input name='pass' type='password' autocomplete='new-password'>"
         "<button type='submit'>Save & Connect</button></form></div>");
  WiFi.scanDelete();

  h += F("<div class='card'><a class='b' href='/network?forget=1'>Forget saved home Wi-Fi</a></div>");
  sendHTML(h+back()+foot());
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
  Serial.println("\nESP32-S3 CYBERDECK TOOLBOX V4.4.2");
  Serial.printf("Flash: %u\n",ESP.getFlashChipSize());
  Serial.printf("PSRAM: %u\n",ESP.getPsramSize());

  Wire.begin(I2C_SDA,I2C_SCL);
  analogReadResolution(12);

  Wire.begin(8,9);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID,AP_PASS);
  connectSavedWiFi();
  loadV4Settings();
  logEvent("Cyberdeck V4 boot");
  BLEDevice::init("S3-Cyberdeck");

  server.on("/",root);
  server.on("/system",systemPage);
  server.on("/network",HTTP_GET,networkPage);
  server.on("/network",HTTP_POST,networkPage);
  server.on("/v43-modules",v43ModulesPage);
  server.on("/storage",storagePage);
  server.on("/cc1101",cc1101Page);
  server.on("/wifi-analyzer",wifiAnalyzerPage);
  server.on("/ble-explorer",bleExplorerPage);
  server.on("/i2c-explorer",i2cExplorerPage);
  server.on("/gpio-lab",gpioLabPage);
  server.on("/monitor",monitorPage);
  server.on("/terminal",HTTP_GET,terminalPage);
  server.on("/terminal",HTTP_POST,terminalPage);
  server.on("/diagnostics",diagnosticsPage);
  server.on("/modules",modulesPage);
  server.on("/pins",pinsPage);
  server.on("/logs",logsPage);
  server.on("/wifi",wifiPage);
  server.on("/ble",blePage);
  server.on("/i2c",i2cPage);
  server.on("/gpio",gpioPage);
  server.on("/adc",adcPage);
  server.on("/pwm",pwmPage);
  server.on("/expansion",expansionPage);
  server.onNotFound([](){server.send(404,"text/plain","404");});
  
  server.on("/uart", HTTP_GET, []() {
    server.send(200, "text/html", uartAnalyzerPage());
  });
  server.on("/uart/start", HTTP_GET, []() {
    int rx = server.hasArg("rx") ? server.arg("rx").toInt() : -1;
    int tx = server.hasArg("tx") ? server.arg("tx").toInt() : -1;
    uint32_t baud = server.hasArg("baud") ? (uint32_t)server.arg("baud").toInt() : 4800;
    if (!startUARTAnalyzer(rx, tx, baud)) {
      server.send(400, "text/plain", "Invalid/reserved RX/TX pins or unsupported baud rate.");
      return;
    }
    server.sendHeader("Location", "/uart");
    server.send(303);
  });
  server.on("/uart/sendtext", HTTP_POST, []() {
    if (!server.hasArg("data") || !uartSendText(server.arg("data"))) {
      server.send(400, "text/plain", "UART not running or empty data.");
      return;
    }
    server.sendHeader("Location", "/uart");
    server.send(303);
  });
  server.on("/uart/sendhex", HTTP_POST, []() {
    if (!server.hasArg("data") || !uartSendHex(server.arg("data"))) {
      server.send(400, "text/plain", "UART not running or invalid HEX.");
      return;
    }
    server.sendHeader("Location", "/uart");
    server.send(303);
  });
  server.on("/uart/stop", HTTP_GET, []() {
    stopUARTAnalyzer();
    server.sendHeader("Location", "/uart");
    server.send(303);
  });
  server.on("/uart/clear", HTTP_GET, []() {
    uartCapture = "";
    server.sendHeader("Location", "/uart");
    server.send(303);
  });

  server.begin();

  Serial.printf("AP: %s\n",AP_SSID);
  Serial.printf("Password: %s\n",AP_PASS);
  Serial.print("Fallback AP: http://"); Serial.println(WiFi.softAPIP());
  if(WiFi.status()==WL_CONNECTED){
    Serial.print("Home LAN: http://"); Serial.println(WiFi.localIP());
  }
}
void loop(){server.handleClient();
  pollUARTAnalyzer();delay(2);}
