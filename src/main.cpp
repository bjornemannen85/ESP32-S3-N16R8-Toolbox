#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <SD_MMC.h>
#include "v6_web.h"
#include "v61_wifi.h"
#include "v61_scripts.h"
#include "v61_bundle2640.h"
#include "v61_bundle4160.h"
#include "v61_bundle6180.h"
#include "v61_bundle81100.h"
#include "v61_bundle101120.h"
#include "v61_ble_transfer.h"
#include "v61_release_guard.h"
#include "v61_microsd.h"
#include "v61_mega.h"
#include "v61_bundle.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ES3C28P: known-good V4.7 display/touch wiring.
constexpr int LCD_CS=10,LCD_DC=46,LCD_SCK=12,LCD_MOSI=11,LCD_MISO=13,LCD_BL=45;
constexpr int TOUCH_SDA=16,TOUCH_SCL=15,TOUCH_RST=18,TOUCH_ADDR=0x38;
SPIClass lcdSPI(FSPI);
Adafruit_ILI9341 lcd(&lcdSPI,LCD_DC,LCD_CS,-1);
constexpr uint16_t BG=0x0008,PANEL=0x1084,ACCENT=0x05FF,WHITE=0xFFFF,MUTED=0x9CF3,ORANGE=0xFD20;
constexpr uint16_t BLUE=0x047F,PURPLE=0xA81F,GREEN=0x07E0,RED=0xF945,YELLOW=0xFFE0;

enum Page { HOME, RADIO, HARDWARE, TOOLS, WIFI, BLE, SYSTEM, TOUCH, NFC, SUBGHZ, IR, PAGE_GPIO, UART, I2C, SPI_DIAG, STORAGE, SETTINGS, LOGS, MODULES, WIFI_SETUP, WIFI_KEYBOARD };
Page page=HOME, parent=HOME;
void draw();
Page history[12]; int historyDepth=0;
void goBack(){page=historyDepth>0?history[--historyDepth]:HOME;draw();}
bool lastPressed=false,touchOK=false,bleReady=false;
bool sdMounted=false; String sdState="NOT TESTED";
uint32_t lastTouch=0,lastSystemDraw=0;
String wifiLines[40],bleLines[40]; int wifiCount=-1,bleCount=-1;
int wifiStored=0,bleStored=0,wifiOffset=0,bleOffset=0;
int wifiRssi[40]={0},bleRssi[40]={0};
const char *names[]={"HOME","RADIO TOOLS","HARDWARE","UTILITIES","WIFI SCANNER","BLE SCANNER","SYSTEM MONITOR","TOUCH TEST","PN532 NFC / EMV","CC1101 SUB-GHZ","IR ANALYZER","GPIO LAB","UART TERMINAL","I2C EXPLORER","SPI DIAGNOSTICS","MICROSD STORAGE","SETTINGS","EVENT LOG","MODULE MANAGER"};
const Page radioItems[]={WIFI,BLE,NFC,SUBGHZ,IR};
const Page hwItems[]={PAGE_GPIO,UART,I2C,SPI_DIAG,STORAGE,MODULES};
const Page toolItems[]={SYSTEM,TOUCH,SETTINGS,LOGS};
String wifiEditSSID="", wifiEditPass="", wifiEditApSSID="", wifiEditApPass="";
String *wifiEditTarget=nullptr; Page wifiReturnPage=WIFI_SETUP;
bool wifiCaps=false; int wifiField=0;
void wifiOpenKeyboard(String &target){wifiEditTarget=&target;page=WIFI_KEYBOARD;draw();}
void wifiKeyboardKey(uint16_t x,uint16_t y){
 if(y>=45&&y<181){
   int row=(y-45)/34,col=x/32;
   const char *rows[]={"1234567890","qwertyuiop","asdfghjkl-","zxcvbnm._@"};
   if(row>=0&&row<4&&col>=0&&col<10){char c=rows[row][col];if(c&&wifiEditTarget&&wifiEditTarget->length()<63){if(wifiCaps&&c>='a'&&c<='z')c-=32;*wifiEditTarget+=c;}draw();}
 }else if(y>=185){
   if(x<80){wifiCaps=!wifiCaps;draw();}
   else if(x<160){if(wifiEditTarget&&wifiEditTarget->length())wifiEditTarget->remove(wifiEditTarget->length()-1);draw();}
   else if(x<240){if(wifiEditTarget&&wifiEditTarget->length()<63)*wifiEditTarget+=' ';draw();}
   else{page=wifiReturnPage;draw();}
 }
}


bool readTouch(uint16_t &x,uint16_t &y){
  Wire.beginTransmission(TOUCH_ADDR);Wire.write(0x02);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom((uint8_t)TOUCH_ADDR,(uint8_t)5)!=(uint8_t)5)return false;
  uint8_t s=Wire.read(),xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();
  if((s&0x0F)==0 || (s&0x0F)>2)return false;
  uint16_t rx=((xh&0x0F)<<8)|xl,ry=((yh&0x0F)<<8)|yl;
  x=constrain((int)ry,0,319);y=constrain(239-(int)rx,0,239);return true;
}
void label(const String &s,int x,int y,uint8_t sz=2,uint16_t color=WHITE){lcd.setTextSize(sz);lcd.setTextColor(color);lcd.setCursor(x,y);lcd.print(s);}
uint16_t tileColor(Page p){switch(p){case WIFI:case RADIO:return ACCENT;case BLE:return BLUE;case NFC:return PURPLE;case SUBGHZ:return ORANGE;case IR:return RED;case PAGE_GPIO:return GREEN;case UART:return ACCENT;case I2C:case SPI_DIAG:return YELLOW;case SYSTEM:return BLUE;case STORAGE:return BLUE;default:return MUTED;}}
void tile(int x,int y,int w,int h,const String &title,Page target){
 uint16_t c=tileColor(target);
 lcd.fillRoundRect(x,y,w,h,6,PANEL);lcd.drawRoundRect(x,y,w,h,6,c);
 int cx=x+w/2,cy=y+17;
 if(target==WIFI || target==RADIO){
   lcd.drawCircle(cx,cy,10,c);lcd.drawCircle(cx,cy,6,c);lcd.fillCircle(cx,cy,2,c);
 }else if(target==BLE){
   lcd.drawLine(cx,cy-10,cx,cy+10,c);lcd.drawLine(cx,cy-10,cx+7,cy-3,c);
   lcd.drawLine(cx+7,cy-3,cx-7,cy+6,c);lcd.drawLine(cx-7,cy-6,cx+7,cy+4,c);
   lcd.drawLine(cx+7,cy+4,cx,cy+10,c);
 }else if(target==SUBGHZ || target==NFC){
   lcd.drawCircle(cx,cy,4,c);lcd.drawCircle(cx,cy,9,c);lcd.fillCircle(cx,cy,2,c);
 }else if(target==SYSTEM || target==TOOLS || target==SETTINGS){
   lcd.drawRoundRect(cx-10,cy-10,20,20,4,c);lcd.drawCircle(cx,cy,5,c);
 }else{
   lcd.drawRoundRect(cx-9,cy-9,18,18,3,c);lcd.fillRect(cx-3,cy-3,6,6,c);
 }
 lcd.setTextSize(1);lcd.setTextColor(WHITE);
 String t=title; if(t.length()>12)t=t.substring(0,12);
 lcd.setCursor(x+(w-(int)t.length()*6)/2,y+h-14);lcd.print(t);
}
void button(int x,int y,int w,int h,const String &s){lcd.fillRoundRect(x,y,w,h,7,PANEL);lcd.drawRoundRect(x,y,w,h,7,ACCENT);label(s,x+10,y+(h-16)/2,2);}
String clipped(String s,size_t n);
void header(const String &s){lcd.fillScreen(BG);lcd.fillRect(0,0,320,33,PANEL);lcd.drawFastHLine(0,33,320,ACCENT);label(clipped(s,25),8,9,1,ACCENT);}
void backButton(){button(8,198,104,35,"BACK");}
void footer(const String &s){label(s,9,235,1,MUTED);}
String clipped(String s,size_t n){if(s.length()>n)s=s.substring(0,n-2)+"..";return s;}
void menu(const Page *items,int count){
 for(int i=0;i<count;i++){int col=i%2,row=i/2;tile(8+col*156,42+row*49,148,43,names[items[i]],items[i]);}
 backButton();
}
void initStorage(){
  // ES3C28P candidate SDMMC pin mapping; verify against board revision.
  SD_MMC.setPins(38,40,39,41,48,47);
  sdMounted=SD_MMC.begin("/sdcard",false);
  sdState=sdMounted?String("READY ")+String((uint32_t)(SD_MMC.cardSize()/1048576ULL))+" MB":"NOT MOUNTED";
  v6Log(String("SD: ")+sdState);
}
void draw(){
 header(page==HOME?"CYBERDECK V6.1 PRO":names[page]);
 switch(page){
 case HOME:{
  const Page homeItems[]={WIFI,BLE,NFC,SUBGHZ,IR,PAGE_GPIO,UART,I2C,STORAGE,SYSTEM,SETTINGS,TOOLS};
  const char* homeNames[]={"WIFI","BLE","NFC/RFID","SUB-GHZ","IR","GPIO","UART","I2C/SPI","SD CARD","SYSTEM","SETTINGS","TOOLS"};
  for(int i=0;i<12;i++){
   int col=i%4,row=i/4;
   tile(5+col*79,39+row*59,73,54,homeNames[i],homeItems[i]);
  }
  label("ES3C28P V6.1 SAFE MODE",6,223,1,MUTED);break;
 }
 case RADIO: menu(radioItems,5);break;
 case HARDWARE: menu(hwItems,6);break;
 case TOOLS: menu(toolItems,4);break;

 case SYSTEM:{
  int vals[]={int(ESP.getCpuFreqMHz()),
    int(ESP.getFreeHeap()/1024),
    int(ESP.getPsramSize()/1048576),
    int(ESP.getFlashChipSize()/1048576)};
  const char* titles[]={"CPU MHz","HEAP KB","PSRAM MB","FLASH MB"};
  uint16_t colors[]={ACCENT,GREEN,PURPLE,ORANGE};
  for(int i=0;i<4;i++){
    int x=8+(i%2)*156,y=42+(i/2)*74;
    lcd.fillRoundRect(x,y,148,66,7,PANEL);
    lcd.drawRoundRect(x,y,148,66,7,colors[i]);
    label(titles[i],x+9,y+9,1,colors[i]);
    label(String(vals[i]),x+9,y+30,2,WHITE);
  }
  backButton();
  break;
 }
 case TOUCH:label("Draw on screen",10,52);backButton();break;
 case WIFI:case BLE:{
  bool isWifi=page==WIFI;
  int total=isWifi?wifiCount:bleCount;
  int stored=isWifi?wifiStored:bleStored;
  int offset=isWifi?wifiOffset:bleOffset;
  label(total<0?"Press SCAN":String(isWifi?"Networks: ":"Devices: ")+String(total),10,40,1,MUTED);
  if(total>stored)label(String("Showing first ")+String(stored),184,40,1,ORANGE);
  for(int i=0;i<5;i++){
    int idx=offset+i;if(idx>=stored)break;
    int y=56+i*26;
    lcd.drawRoundRect(8,y,304,24,4,BLUE);
    String name=isWifi?wifiLines[idx]:bleLines[idx];
    int rssi=isWifi?wifiRssi[idx]:bleRssi[idx];
    label(clipped(name,29),13,y+4,1,WHITE);
    label(String(rssi),245,y+4,1,MUTED);
    int bars=rssi>=-55?4:rssi>=-67?3:rssi>=-78?2:rssi>=-90?1:0;
    for(int j=0;j<4;j++)lcd.fillRect(215+j*6,y+17-j*3,4,3+j*3,j<bars?GREEN:MUTED);
  }
  backButton();
  button(119,198,56,35,"<");
  button(179,198,56,35,">");
  button(240,198,72,35,"SCAN");
  break;
 }
 case MODULES:
  label("PN532: NOT CONFIGURED",8,52,1,ORANGE);
  label("CC1101: NOT CONFIGURED",8,76,1,ORANGE);
  label("IR: NOT CONFIGURED",8,100,1,ORANGE);
  label("SD: "+sdState,8,124,1,sdMounted?GREEN:ORANGE);
  label("GPIO: NOT CONFIGURED",8,155,1,MUTED);backButton();break;
 case STORAGE:
  label("INTEGRATED MICROSD",12,48,1,ACCENT);
  label("STATUS: "+sdState,12,74,1,sdMounted?GREEN:ORANGE);
  if(sdMounted){
    label("TOTAL: "+String((uint32_t)(SD_MMC.totalBytes()/1048576ULL))+" MB",12,101,1);
    label("USED: "+String((uint32_t)(SD_MMC.usedBytes()/1048576ULL))+" MB",12,123,1);
  }
  backButton();break;
 case NFC:case SUBGHZ:case IR:case PAGE_GPIO:case UART:case I2C:case SPI_DIAG:
  label("NOT CONFIGURED",12,65,2,ORANGE);
  label("Hardware / pins pending",12,103,1,MUTED);
  label("No GPIO activity",12,122,1,MUTED);backButton();break;
 case WIFI_SETUP:{
   label("WIFI SETUP (TOUCH)",8,39,1,ACCENT);
   label("HOME SSID: "+clipped(wifiEditSSID,19),8,57,1);
   label("HOME PASS: "+String(wifiEditPass.length(),'*'),8,76,1);
   label("AP SSID: "+clipped(wifiEditApSSID,19),8,95,1);
   label("AP PASS: "+String(wifiEditApPass.length(),'*'),8,114,1);
   label("Tap field to edit",8,133,1,MUTED);
   button(8,151,145,35,"SAVE STA");
   button(161,151,151,35,"SAVE AP");
   backButton();break;
 }
 case WIFI_KEYBOARD:{
   label("EDIT: "+String(wifiField==0?"HOME SSID":wifiField==1?"HOME PASS":wifiField==2?"AP SSID":"AP PASS"),8,35,1,ACCENT);
   const char *rows[]={"1234567890","qwertyuiop","asdfghjkl-","zxcvbnm._@"};
   for(int r=0;r<4;r++)for(int c=0;c<10;c++){
     char k=rows[r][c];if(!k)continue;
     int x=c*32,y=45+r*34;lcd.drawRoundRect(x+1,y+1,30,31,3,ACCENT);
     if(wifiCaps&&k>='a'&&k<='z')k-=32;
     label(String(k),x+10,y+9,1);
   }
   label(wifiCaps?"ABC":"abc",8,195,1,ACCENT);
   label("DEL",91,195,1,ACCENT);
   label("SPACE",168,195,1,ACCENT);
   label("DONE",253,195,1,GREEN);break;
 }
 case SETTINGS:
  label("WEB DASHBOARD ACTIVE",12,126,1,GREEN);
  label("SSID: "+v6SSID,12,144,1,WHITE);
  label("PASS: "+v6Password,12,158,1,WHITE);
  label("OPEN: 192.168.4.1",12,173,1,ACCENT);
  label("ADMIN TOKEN:",12,43,1,ORANGE);
  label(V61WiFi::adminToken,12,57,1,WHITE);
  label("WiFi manager: /wifi",12,79,1,GREEN);
  label("Tap WIFI SETUP below",12,101,1,GREEN);backButton();button(121,198,191,35,"WIFI SETUP");break;
 case LOGS:
  label("Runtime event log",12,58,1);
  label("Boot OK",12,80,1,MUTED);
  label(touchOK?"Touch OK":"Touch unavailable",12,102,1,MUTED);backButton();break;
 }
}
void scanWifi(){v6Log("WiFi scan started");
 header("WIFI SCANNING...");
 for(auto &line:wifiLines)line="";
 wifiStored=0;wifiOffset=0;
 WiFi.mode(WIFI_AP_STA);delay(100);
 int n=WiFi.scanNetworks(false,true);wifiCount=n<0?0:n;
 wifiStored=min(wifiCount,40);
 for(int i=0;i<wifiStored;i++){
   String name=WiFi.SSID(i);if(!name.length())name="<hidden>";
   wifiLines[i]=clipped(name,28)+" ch"+String(WiFi.channel(i));
   wifiRssi[i]=WiFi.RSSI(i);
 }
 WiFi.scanDelete();draw();
}
void scanBle(){v6Log("BLE scan started");
 header("BLE SCANNING...");
 for(auto &line:bleLines)line="";
 bleStored=0;bleOffset=0;
 if(!bleReady){BLEDevice::init("");bleReady=true;}
 BLEScan *scanner=BLEDevice::getScan();scanner->setActiveScan(false);
 BLEScanResults results=scanner->start(4,false);
 bleCount=results.getCount();bleStored=min(bleCount,40);
 for(int i=0;i<bleStored;i++){
   BLEAdvertisedDevice d=results.getDevice(i);
   bleLines[i]=d.haveName()?String(d.getName().c_str()):String(d.getAddress().toString().c_str());
   bleRssi[i]=d.getRSSI();
 }
 scanner->clearResults();draw();
}
void enter(Page p,Page from){if(historyDepth<12)history[historyDepth++]=page;parent=from;page=p;draw();}
void setup(){Serial.begin(115200);delay(250);pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,HIGH);lcdSPI.begin(LCD_SCK,LCD_MISO,LCD_MOSI,LCD_CS);lcd.begin(40000000);lcd.setRotation(1);lcd.invertDisplay(true);pinMode(TOUCH_RST,OUTPUT);digitalWrite(TOUCH_RST,LOW);delay(15);digitalWrite(TOUCH_RST,HIGH);delay(120);Wire.begin(TOUCH_SDA,TOUCH_SCL,100000);Wire.beginTransmission(TOUCH_ADDR);touchOK=Wire.endTransmission()==0;WiFi.mode(WIFI_OFF);v6Start();V61WiFi::start();v6SSID=V61WiFi::apSSID;v6Password=V61WiFi::apPassword;wifiEditApSSID=v6SSID;V61WiFi::routes(v6Server);V61Scripts::begin();V61Scripts::routes(v6Server);V61Bundle2640::routes(v6Server);V61Bundle4160::routes(v6Server);V61Bundle6180::routes(v6Server);V61Bundle81100::routes(v6Server);V61Bundle101120::routes(v6Server);V61BleTransfer::routes(v6Server);V61ReleaseGuard::routes(v6Server);V61MicroSD::routes(v6Server);V61Mega::routes(v6Server);V61Bundle::routes(v6Server);/* SDMMC pin map not yet verified: initStorage() disabled */ Serial.printf("ES3C28P V4.9.1 touch=%s\n",touchOK?"YES":"NO");draw();}
void loop(){
 v6Server.handleClient();V61WiFi::tick();V61Bundle81100::tick();V61BleTransfer::tick();
 uint16_t x=0,y=0;bool pressed=readTouch(x,y);
 if(pressed&&page==TOUCH&&y<185)lcd.fillCircle(x,y,3,BLUE);
 if(pressed&&!lastPressed&&millis()-lastTouch>220){
  lastTouch=millis();
  if(page==HOME){
   if(x>=5&&x<317&&y>=39&&y<211){
    int col=(int(x)-5)/79,row=(int(y)-39)/59;
    int lx=(int(x)-5)%79,ly=(int(y)-39)%59;
    if(col<4&&row<3&&lx<73&&ly<54){
     const Page homeItems[]={WIFI,BLE,NFC,SUBGHZ,IR,PAGE_GPIO,UART,I2C,STORAGE,SYSTEM,SETTINGS,TOOLS};
     enter(homeItems[row*4+col],HOME);
    }
   }
  }else if(page==WIFI_KEYBOARD){wifiKeyboardKey(x,y);}
  else if(page==WIFI_SETUP){
    if(y>=190&&x<125){page=SETTINGS;draw();}
    else if(y>=55&&y<130){
      wifiField=(y-55)/19;
      if(wifiField>3)wifiField=3;
      String *fields[]={&wifiEditSSID,&wifiEditPass,&wifiEditApSSID,&wifiEditApPass};
      wifiOpenKeyboard(*fields[wifiField]);
    }else if(y>=151&&y<190){
      if(x<156){
        if(V61WiFi::validSSID(wifiEditSSID)&&wifiEditPass.length()>=8){
          V61WiFi::upsert(wifiEditSSID,wifiEditPass,10);v6Log("STA profile saved");
        }else v6Log("STA fields invalid");
      }else{
        if(V61WiFi::setAP(wifiEditApSSID,wifiEditApPass)){
          v6SSID=V61WiFi::apSSID;v6Password=V61WiFi::apPassword;v6Log("AP settings saved");
        }else v6Log("AP fields invalid");
      }draw();
    }
  }else if(page==SETTINGS&&y>=190&&x>=120){
    wifiEditApSSID=V61WiFi::apSSID;wifiEditApPass="";
    page=WIFI_SETUP;draw();
  }else if(y>=190&&x<125){goBack();}
  else if(page==RADIO||page==HARDWARE||page==TOOLS){
   const Page *items=page==RADIO?radioItems:page==HARDWARE?hwItems:toolItems;
   int count=page==RADIO?5:page==HARDWARE?6:4;
   if(y>=42&&y<183){int row=(int(y)-42)/49,col=x<160?0:1,index=row*2+col;
    if(index>=0&&index<count&&y<42+row*49+43)enter(items[index],page);
   }
  }else if((page==WIFI||page==BLE)&&y>=195&&x>=119){
    if(x>=240){if(page==WIFI)scanWifi();else scanBle();}
    else{
      int &offset=page==WIFI?wifiOffset:bleOffset;
      int stored=page==WIFI?wifiStored:bleStored;
      if(x<179)offset=max(0,offset-5);
      else if(offset+5<stored)offset+=5;
      draw();
    }
  }
 }
 lastPressed=pressed;
 if(page==SYSTEM&&millis()-lastSystemDraw>2500){lastSystemDraw=millis();draw();}
 delay(25);
}
