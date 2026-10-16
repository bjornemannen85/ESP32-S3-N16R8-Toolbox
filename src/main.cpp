#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ES3C28P: preserve the V4.6 display and touch wiring verified on hardware.
static constexpr int LCD_CS=10,LCD_DC=46,LCD_SCK=12,LCD_MOSI=11,LCD_MISO=13,LCD_BL=45;
static constexpr int TOUCH_SDA=16,TOUCH_SCL=15,TOUCH_RST=18,TOUCH_ADDR=0x38;
SPIClass lcdSPI(FSPI);
Adafruit_ILI9341 lcd(&lcdSPI,LCD_DC,LCD_CS,-1);
static constexpr uint16_t BG=0x0841,PANEL=0x18E3,ACCENT=0x07FF,WHITE=0xFFFF,MUTED=0xBDF7,ORANGE=0xFD20;
enum Page { HOME,SYSTEM,TOUCH,WIFI,BLE,MODULES };
Page page=HOME;
bool lastPressed=false,touchOK=false,bleReady=false;
uint32_t lastTouch=0,lastSystemDraw=0;
String wifiLines[7],bleLines[7];
int wifiCount=-1,bleCount=-1;

bool readTouch(uint16_t &x,uint16_t &y){
  Wire.beginTransmission(TOUCH_ADDR);Wire.write(0x02);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(TOUCH_ADDR,(uint8_t)5)!=(uint8_t)5)return false;
  uint8_t s=Wire.read(),xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();
  if((s&0x0F)==0 || (s&0x0F)>2)return false;
  uint16_t rx=((xh&0x0F)<<8)|xl,ry=((yh&0x0F)<<8)|yl;
  x=constrain((int)ry,0,319);y=constrain(239-(int)rx,0,239);return true;
}
void label(const String &s,int x,int y,uint8_t sz=2,uint16_t color=WHITE){lcd.setTextSize(sz);lcd.setTextColor(color);lcd.setCursor(x,y);lcd.print(s);}
void button(int x,int y,int w,int h,const String &s){lcd.fillRoundRect(x,y,w,h,8,PANEL);lcd.drawRoundRect(x,y,w,h,8,ACCENT);label(s,x+10,y+(h-16)/2,2);}
void header(const String &s){lcd.fillScreen(BG);lcd.fillRect(0,0,320,37,PANEL);label(s,10,10,2,ACCENT);}
void backButton(){button(8,192,104,39,"BACK");}
void footer(const String &s){label(s,10,231,1,MUTED);}
String clipped(String s,size_t maxLen){if(s.length()>maxLen)s=s.substring(0,maxLen-2)+"..";return s;}
void draw(){
  switch(page){
    case HOME:
      header("CYBERDECK V4.7");
      button(8,46,148,46,"SYSTEM");button(164,46,148,46,"TOUCH");
      button(8,100,148,46,"WIFI");button(164,100,148,46,"BLE");
      button(8,154,304,43,"MODULES");footer("ES3C28P  |  TOUCH EDITION");break;
    case SYSTEM:
      header("SYSTEM MONITOR");
      label("Flash: "+String(ESP.getFlashChipSize()/1048576)+" MB",10,47);
      label("PSRAM: "+String(ESP.getPsramSize()/1048576)+" MB",10,75);
      label("Heap: "+String(ESP.getFreeHeap()/1024)+" KB",10,103);
      label("Uptime: "+String(millis()/1000)+" s",10,131);
      label("CPU: "+String(ESP.getCpuFreqMHz())+" MHz",10,159,1,MUTED);
      backButton();break;
    case TOUCH:
      header("TOUCH TEST");label("Draw on screen",10,52);
      backButton();break;
    case WIFI:
      header("WIFI SCANNER");
      label(wifiCount<0?"Press SCAN":String("Networks: ")+wifiCount,10,43,1,MUTED);
      for(int i=0;i<7;i++)if(wifiLines[i].length())label(wifiLines[i],10,61+i*18,1);
      button(8,192,104,39,"BACK");button(174,192,138,39,"SCAN");break;
    case BLE:
      header("BLE SCANNER");
      label(bleCount<0?"Press SCAN":String("Devices: ")+bleCount,10,43,1,MUTED);
      for(int i=0;i<7;i++)if(bleLines[i].length())label(bleLines[i],10,61+i*18,1);
      button(8,192,104,39,"BACK");button(174,192,138,39,"SCAN");break;
    case MODULES:
      header("MODULE MANAGER");
      label("PN532 NFC: NOT CONFIGURED",8,55,1,ORANGE);
      label("CC1101 RF: NOT CONFIGURED",8,81,1,ORANGE);
      label("SD: NOT TESTED",8,107,1,MUTED);
      label("GPIO pinout must be checked",8,145,1);
      label("before connecting modules.",8,158,1);
      backButton();break;
  }
}
void scanWifi(){
  header("WIFI SCANNING...");label("Passive discovery",10,60,2);
  for(auto &s:wifiLines)s="";
  WiFi.mode(WIFI_STA);WiFi.disconnect(false,false);delay(100);
  int n=WiFi.scanNetworks(false,true);wifiCount=n<0?0:n;
  for(int i=0;i<n && i<7;i++){
    String ssid=WiFi.SSID(i);if(ssid.length()==0)ssid="<hidden>";
    wifiLines[i]=clipped(ssid,20)+" "+String(WiFi.RSSI(i))+"dBm ch"+String(WiFi.channel(i));
  }
  WiFi.scanDelete();draw();
}
void scanBle(){
  header("BLE SCANNING...");label("Listening for 4 seconds",10,60,1);
  for(auto &s:bleLines)s="";
  if(!bleReady){BLEDevice::init("");bleReady=true;}
  BLEScan *scanner=BLEDevice::getScan();scanner->setActiveScan(false);
  BLEScanResults results=scanner->start(4,false);
  bleCount=results.getCount();
  for(int i=0;i<bleCount && i<7;i++){
    BLEAdvertisedDevice d=results.getDevice(i);
    String name=d.haveName()?String(d.getName().c_str()):String(d.getAddress().toString().c_str());
    bleLines[i]=clipped(name,24)+" "+String(d.getRSSI())+"dBm";
  }
  scanner->clearResults();draw();
}
void setup(){
  Serial.begin(115200);delay(250);
  pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,HIGH);
  lcdSPI.begin(LCD_SCK,LCD_MISO,LCD_MOSI,LCD_CS);
  lcd.begin(40000000);lcd.setRotation(1);
  pinMode(TOUCH_RST,OUTPUT);digitalWrite(TOUCH_RST,LOW);delay(15);digitalWrite(TOUCH_RST,HIGH);delay(120);
  Wire.begin(TOUCH_SDA,TOUCH_SCL,100000);
  Wire.beginTransmission(TOUCH_ADDR);touchOK=Wire.endTransmission()==0;
  WiFi.mode(WIFI_OFF);
  Serial.printf("ES3C28P V4.7: touch=%s flash=%u psram=%u\n",touchOK?"YES":"NO",ESP.getFlashChipSize(),ESP.getPsramSize());
  draw();
}
void loop(){
  uint16_t x=0,y=0;bool pressed=readTouch(x,y);
  if(pressed && page==TOUCH && y<185)lcd.fillCircle(x,y,3,ILI9341_BLUE);
  if(pressed && !lastPressed && millis()-lastTouch>220){
    lastTouch=millis();
    if(page==HOME){
      if(y>=46&&y<92)page=x<160?SYSTEM:TOUCH;
      else if(y>=100&&y<146)page=x<160?WIFI:BLE;
      else if(y>=154&&y<197)page=MODULES;
      draw();
    }else if(y>=192&&x<120){page=HOME;draw();}
    else if(y>=192&&x>=174&&page==WIFI)scanWifi();
    else if(y>=192&&x>=174&&page==BLE)scanBle();
  }
  lastPressed=pressed;
  if(page==SYSTEM && millis()-lastSystemDraw>2500){lastSystemDraw=millis();draw();}
  delay(25);
}
