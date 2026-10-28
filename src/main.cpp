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
#include "v61_usb_hid.h"
#include "v61_microsd.h"
#include "v61_mega.h"
#include "v61_bundle.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "v62_wolfdeck.h"
#include "v62_categories.h"
#include "v62_keyboard.h"
#include "v62_icons.h"
#include "v62_scroll.h"
#include "v62_app_content.h"

// ES3C28P: known-good V4.7 display/touch wiring.
constexpr int LCD_CS=10,LCD_DC=46,LCD_SCK=12,LCD_MOSI=11,LCD_MISO=13,LCD_BL=45;
constexpr int TOUCH_SDA=16,TOUCH_SCL=15,TOUCH_RST=18,TOUCH_ADDR=0x38;
SPIClass lcdSPI(FSPI);
Adafruit_ILI9341 lcd(&lcdSPI,LCD_DC,LCD_CS,-1);
constexpr uint16_t BG=0x0008,PANEL=0x1084,ACCENT=0x05FF,WHITE=0xFFFF,MUTED=0x9CF3,ORANGE=0xFD20;
constexpr uint16_t BLUE=0x047F,PURPLE=0xA81F,GREEN=0x07E0,RED=0xF945,YELLOW=0xFFE0;

enum Page { HOME, RADIO, HARDWARE, TOOLS, WIFI, BLE, SYSTEM, TOUCH, NFC, SUBGHZ, IR, PAGE_GPIO, UART, I2C, SPI_DIAG, STORAGE, SETTINGS, LOGS, MODULES, WIFI_SETUP, WIFI_KEYBOARD, PROGRAMS, PROGRAM_ACTION, PROGRAM_RESULT, NETWORK_TOOLS, BLUETOOTH_TOOLS, FILE_TOOLS, SYSTEM_TOOLS, SETTINGS_TOOLS, BADUSB_FILES, UART_FILES, SCRIPT_INFO, PAGE_COUNT };
Page page=HOME, parent=HOME;
void draw();
void v62SaveNavigation(int index);
void v62RestoreNavigation(int index);
Page history[12]; int historyDepth=0;
void goBack(){int index=historyDepth>0?--historyDepth:-1;page=index>=0?history[index]:HOME;if(page==PROGRAMS)v62RestoreNavigation(index);draw();}
bool lastPressed=false,touchOK=false,bleReady=false;
bool sdMounted=false; String sdState="NOT TESTED";
uint32_t lastTouch=0,lastSystemDraw=0;
String wifiLines[40],bleLines[40]; int wifiCount=-1,bleCount=-1;
int wifiStored=0,bleStored=0,wifiOffset=0,bleOffset=0;
int wifiRssi[40]={0},bleRssi[40]={0};
const char *names[]={"HOME","RADIO TOOLS","HARDWARE","UTILITIES","WIFI SCANNER","BLE SCANNER","SYSTEM MONITOR","TOUCH TEST","PN532 NFC / EMV","CC1101 SUB-GHZ","IR ANALYZER","GPIO LAB","UART TERMINAL","I2C EXPLORER","SPI DIAGNOSTICS","MICROSD STORAGE","SETTINGS","EVENT LOG","MODULE MANAGER","WIFI SETUP","WIFI KEYBOARD","ALL PROGRAMS","PROGRAM","RESULT","NETWORK","BLUETOOTH","FILES / SCRIPTS","SYSTEM TOOLS","SETTINGS TOOLS","BADUSB SCRIPTS","UART SCRIPTS","SCRIPT GUIDE"};
static_assert(sizeof(names)/sizeof(names[0])==PAGE_COUNT,"Every screen must have a title");
void enter(Page p,Page from);
void v62Draw();
void v62Touch(uint16_t x,uint16_t y);
void v62Open(const String &filter);
void v62OpenCategory(int category);
String v62MenuTitle();
bool v62ScrollTouch(bool down,uint16_t x,uint16_t y);
static bool scrollHeld=false;
static String v62ModuleContext="nfc";
void libraryDraw();
bool libraryScroll(bool down,uint16_t x,uint16_t y);
void libraryTouch(uint16_t x,uint16_t y);
void libraryOpen(Page target);
const Page radioItems[]={NFC,SUBGHZ,IR,MODULES};
const Page homeItems[]={NETWORK_TOOLS,BLUETOOTH_TOOLS,HARDWARE,FILE_TOOLS,SYSTEM_TOOLS,SETTINGS_TOOLS,RADIO,PROGRAMS};
const char *homeTitles[]={"NETWORK","BLUETOOTH","HARDWARE","FILES","SYSTEM","SETTINGS","RADIO","ALL TOOLS"};
const char *homeSubtitles[]={"WiFi / NAT / diagnostics","Scan / file receiver","UART / ADC / I2C / USB","Scripts / backup / SD","Monitor / logs / touch","WiFi setup / profiles","NFC / CC1101 / IR","Complete program list"};
const Page hwItems[]={PAGE_GPIO,UART,I2C,SPI_DIAG,STORAGE,MODULES};
const Page toolItems[]={PROGRAMS,SYSTEM,TOUCH,SETTINGS,LOGS,HARDWARE};
String wifiEditSSID="", wifiEditPass="", wifiEditApSSID="", wifiEditApPass="";
String *wifiEditTarget=nullptr; Page wifiReturnPage=WIFI_SETUP;
int wifiField=0;
int wifiKeyboardMode=0;
String v62KeyTitle="";
const char *wifiKeyRows[3][4]={
 {"1234567890","qwertyuiop","asdfghjkl-","zxcvbnm._@"},
 {"1234567890","QWERTYUIOP","ASDFGHJKL-","ZXCVBNM._@"},
 {"!@#$%^&*()","[]{}<>/\\|=","+-_:;,.?~`","\"'01234567"}
};
V62Keys::Input wifiKeys;
int wifiActiveKey=-1;
void wifiKeyboardPreview();
void wifiKeyHighlight(int key);
String v62Mask(size_t count){String out;out.reserve(min(count,size_t(50)));for(size_t i=0;i<count&&i<50;i++)out+='*';return out;}
void wifiOpenKeyboard(String &target){
 wifiEditTarget=&target;wifiKeys.reset();wifiActiveKey=-1;page=WIFI_KEYBOARD;draw();
}
void wifiKeyboardKey(uint16_t x,uint16_t y){
 int key=V62Keys::hit(x,y);if(key<0)return;
 wifiKeyHighlight(key);
 size_t limit=wifiReturnPage==PROGRAM_ACTION?8192u:63u;
 if(key<40){
  char c=wifiKeyRows[wifiKeyboardMode][key/10][key%10];
  if(c&&wifiEditTarget&&wifiEditTarget->length()<limit)*wifiEditTarget+=c;
 }else if(key==40){wifiKeyboardMode=(wifiKeyboardMode+1)%3;wifiActiveKey=-1;draw();return;}
 else if(key==41){if(wifiEditTarget&&wifiEditTarget->length())wifiEditTarget->remove(wifiEditTarget->length()-1);}
 else if(key==42||key==44){
  if(wifiEditTarget&&wifiEditTarget->length()<limit)*wifiEditTarget+=(key==44&&wifiField==4&&v62KeyTitle=="content"?'\n':' ');
 }else if(key==43){wifiEditTarget=nullptr;wifiActiveKey=-1;page=wifiReturnPage;draw();return;}
 wifiKeyboardPreview();
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
void wifiKeyboardPreview(){
 String value=wifiEditTarget?*wifiEditTarget:String("");
 bool secret=wifiField==1||wifiField==3||(wifiField==4&&(v62KeyTitle=="password"||v62KeyTitle=="token"));
 if(secret)value=v62Mask(value.length());
 value.replace("\n"," ");if(value.length()>49)value=value.substring(value.length()-49);
 lcd.fillRect(0,34,320,10,PANEL);label(value,8,35,1,WHITE);
}
void wifiKeyHighlight(int key){
 if(wifiActiveKey>=0&&wifiActiveKey<40){int x=(wifiActiveKey%10)*32,y=45+(wifiActiveKey/10)*34;lcd.drawRoundRect(x+1,y+1,30,31,3,ACCENT);}
 wifiActiveKey=key;
 if(key>=0&&key<40){int x=(key%10)*32,y=45+(key/10)*34;lcd.drawRoundRect(x+1,y+1,30,31,3,GREEN);}
}
uint16_t tileColor(Page p){switch(p){case WIFI:case RADIO:return ACCENT;case BLE:return BLUE;case NFC:return PURPLE;case SUBGHZ:return ORANGE;case IR:return RED;case PAGE_GPIO:return GREEN;case UART:return ACCENT;case I2C:case SPI_DIAG:return YELLOW;case SYSTEM:return BLUE;case STORAGE:return BLUE;default:return MUTED;}}
void tile(int x,int y,int w,int h,const String &title,Page target){
 uint16_t c=tileColor(target);
 WolfDeck::panel(lcd,x,y,w,h);lcd.drawRoundRect(x,y,w,h,6,c);
 V62Icons::draw(lcd,V62Icons::pagesIcon(target),x+5,y+8);
 lcd.setTextSize(1);lcd.setTextColor(WHITE);
 String t=title;if(t.length()>17)t=t.substring(0,15)+"..";
 lcd.setCursor(x+34,y+(h-8)/2);lcd.print(t);
}
void button(int x,int y,int w,int h,const String &s){lcd.fillRoundRect(x,y,w,h,7,PANEL);lcd.drawRoundRect(x,y,w,h,7,ACCENT);label(s,x+10,y+(h-16)/2,2);}
String clipped(String s,size_t n);
void header(const String &s){WolfDeck::background(lcd);lcd.fillRect(0,0,320,33,PANEL);lcd.drawFastHLine(0,33,320,ACCENT);V62Icons::draw(lcd,V62Icons::pagesIcon(page),6,4);label(clipped(s,43),40,13,1,ACCENT);}
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
 if(page==WIFI_KEYBOARD)header("EDIT "+String(wifiField==0?"HOME SSID":wifiField==1?"HOME PASS":wifiField==2?"AP SSID":wifiField==3?"AP PASS":v62KeyTitle));
 else header(page==HOME?"WOLF DECK V6.2":page==PROGRAMS?v62MenuTitle():(page<PAGE_COUNT?names[page]:"UNKNOWN"));
 switch(page){
 case HOME:{
  for(int i=0;i<8;i++){
   int x=V62Menu::HomeX+(i%2)*V62Menu::HomeDX,y=V62Menu::HomeY+(i/2)*V62Menu::HomeDY;
   WolfDeck::panel(lcd,x,y,V62Menu::HomeW,V62Menu::HomeH);
   lcd.drawRoundRect(x,y,V62Menu::HomeW,V62Menu::HomeH,5,ACCENT);
   V62Icons::draw(lcd,V62Icons::homeIcon(i),x+5,y+1);label(homeTitles[i],x+35,y+7,2,WHITE);label(clipped(homeSubtitles[i],22),x+8,y+27,1,MUTED);
  }
  label("V6.2 FULL ACCESS",6,223,1,MUTED);break;
 }
 case RADIO: menu(radioItems,4);break;
 case HARDWARE: menu(hwItems,6);break;
 case TOOLS: menu(toolItems,6);break;
 case PROGRAMS:case PROGRAM_ACTION:case PROGRAM_RESULT:v62Draw();break;
 case BADUSB_FILES:case UART_FILES:case SCRIPT_INFO:libraryDraw();break;

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
    lcd.drawRoundRect(8,y,290,24,4,BLUE);
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
  V62Scroll::Bar{size_t(stored),5,size_t(offset)}.draw(lcd,PANEL,ACCENT);
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
   label("HOME PASS: "+v62Mask(wifiEditPass.length()),8,76,1);
   label("AP SSID: "+clipped(wifiEditApSSID,19),8,95,1);
   label("AP PASS: "+v62Mask(wifiEditApPass.length()),8,114,1);
   label("Tap field to edit",8,133,1,MUTED);
   button(8,151,145,35,"SAVE STA");
   button(161,151,151,35,"SAVE AP");
   backButton();break;
 }
 case WIFI_KEYBOARD:{
   wifiKeyboardPreview();
   const char **rows=wifiKeyRows[wifiKeyboardMode];
   for(int r=0;r<4;r++)for(int c=0;c<10;c++){
     char k=rows[r][c];if(!k)continue;
     int x=c*32,y=45+r*34;lcd.drawRoundRect(x+1,y+1,30,31,3,ACCENT);

     label(String(k),x+10,y+9,1);
   }
   for(int c=0;c<4;c++)lcd.drawRoundRect(c*80+2,186,76,38,4,ACCENT);
   label(wifiKeyboardMode==2?"SYM":wifiKeyboardMode==1?"ABC":"abc",8,195,1,ACCENT);
   label("DEL",91,195,1,ACCENT);
   label("SPACE",166,193,1,ACCENT);
   if(wifiField==4&&v62KeyTitle=="content")label("NEW LINE",166,211,1,MUTED);
   label("DONE",253,195,1,GREEN);break;
 }
 case SETTINGS:
  label("WEB DASHBOARD ACTIVE",12,126,1,GREEN);
  label("SSID: "+V61WiFi::apSSID,12,144,1,WHITE);
  label("PASS: "+V61WiFi::apPassword,12,158,1,WHITE);
  label("OPEN: 192.168.4.1",12,173,1,ACCENT);
  label("WiFi manager: /wifi",12,79,1,GREEN);
  label("Tap WIFI SETUP below",12,101,1,GREEN);backButton();button(121,198,191,35,"WIFI SETUP");break;
 case LOGS:{
  int row=0;for(int i=0;i<12;i++){int j=(v6EventPos+i)%12;if(v6Events[j].length())label(clipped(v6Events[j],49),8,43+row++*12,1,MUTED);}
  backButton();break;
 }
 default:page=HOME;draw();break;
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
 if(V61BleTransfer::running){v6Log("Stop BLE receiver before scanning");draw();return;}
 header("BLE SCANNING...");
 for(auto &line:bleLines)line="";
 bleStored=0;bleOffset=0;
 if(!bleReady){BLEDevice::init("");bleReady=true;}
 BLEScan *scanner=BLEDevice::getScan();scanner->setActiveScan(false);
 #if defined(V61_IDF_NAT_BUILD) && V61_IDF_NAT_BUILD
 BLEScanResults *results=scanner->start(4,false);
 bleCount=results ? results->getCount() : 0;
#else
 BLEScanResults results=scanner->start(4,false);
 bleCount=results.getCount();
#endif
 bleStored=min(bleCount,40);
 for(int i=0;i<bleStored;i++){
   #if defined(V61_IDF_NAT_BUILD) && V61_IDF_NAT_BUILD
   BLEAdvertisedDevice d=results->getDevice(i);
#else
   BLEAdvertisedDevice d=results.getDevice(i);
#endif
   bleLines[i]=d.haveName()?String(d.getName().c_str()):String(d.getAddress().toString().c_str());
   bleRssi[i]=d.getRSSI();
 }
 scanner->clearResults();draw();
}
// Catalog uses the same handlers as web, including authentication and validation.
#include "v62_ui.h"
#include "v62_script_touch.h"
void libraryOpen(Page target){libraryFolderUart=target==UART_FILES;libraryOffset=0;if(historyDepth<12){v62SaveNavigation(historyDepth);history[historyDepth++]=page;}page=target;draw();}
void enter(Page p,Page from){
 if(p==BADUSB_FILES||p==UART_FILES){libraryOpen(p);return;}
 if(p==NFC||p==SUBGHZ||p==IR||p==MODULES){v62ModuleContext=p==NFC?"nfc":p==SUBGHZ?"subghz":p==IR?"ir":"nfc";v62Open("/apps/modules/");return;}
 int category=-1;
 if(p==NETWORK_TOOLS)category=V62Menu::Network;
 else if(p==BLUETOOTH_TOOLS)category=V62Menu::Bluetooth;
 else if(p==HARDWARE)category=V62Menu::Hardware;
 else if(p==FILE_TOOLS)category=V62Menu::Files;
 else if(p==SYSTEM_TOOLS)category=V62Menu::System;
 else if(p==SETTINGS_TOOLS)category=V62Menu::Settings;
 else if(p==RADIO)category=V62Menu::Modules;
 if(category>=0){v62OpenCategory(category);return;}
 if(p==WIFI_SETUP){wifiEditApSSID=V61WiFi::apSSID;wifiEditApPass="";}
 String filter="";
 if(p==UART)filter="/uart/";
 else if(p==I2C)filter="i2c";
 else if(p==STORAGE)filter="/sd/";
 else if(p==PAGE_GPIO)filter="/adc/";
 else if(p==SPI_DIAG)filter="/hardware/plan";
 if(filter.length()){v62Open(filter);return;}
 if(p==PROGRAMS){v62OpenCategory(V62Menu::All);return;}
 if(historyDepth<12){v62SaveNavigation(historyDepth);history[historyDepth++]=page;}
 parent=from;page=p;draw();}
void setup(){
 Serial.begin(115200);delay(250);
 pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,HIGH);
 lcdSPI.begin(LCD_SCK,LCD_MISO,LCD_MOSI,LCD_CS);
 lcd.begin(40000000);lcd.setRotation(1);lcd.invertDisplay(true);
 WolfDeck::bootBegin(lcd);WolfDeck::bootStep(lcd,15,"DISPLAY READY");
 pinMode(TOUCH_RST,OUTPUT);digitalWrite(TOUCH_RST,LOW);delay(15);
 digitalWrite(TOUCH_RST,HIGH);delay(120);
 Wire.begin(TOUCH_SDA,TOUCH_SCL,100000);Wire.beginTransmission(TOUCH_ADDR);
 touchOK=Wire.endTransmission()==0;
 WolfDeck::bootStep(lcd,30,touchOK?"TOUCH READY":"TOUCH NOT DETECTED");
 WiFi.mode(WIFI_OFF);v6Start();WolfDeck::bootStep(lcd,45,"WEB SERVER STARTED");
 V61WiFi::start();v6SSID=V61WiFi::apSSID;v6Password=V61WiFi::apPassword;
 wifiEditApSSID=v6SSID;V61WiFi::routes(v6Server);
 WolfDeck::bootStep(lcd,60,"WIFI / NAT CONFIG LOADED");
 V61Scripts::begin();V61Scripts::routes(v6Server);V62ScriptApps::seed();V62Modules::load();
 WolfDeck::bootStep(lcd,75,V61Scripts::ready?"SCRIPT STORAGE READY":"SCRIPT STORAGE UNAVAILABLE");
 V61Bundle2640::routes(v6Server);V61Bundle4160::routes(v6Server);
 V61Bundle6180::routes(v6Server);V61Bundle81100::routes(v6Server);
 V61Bundle101120::routes(v6Server);V61BleTransfer::routes(v6Server);
 V61ReleaseGuard::routes(v6Server);V61USBHID::routes(v6Server);
 V61MicroSD::routes(v6Server);V61Mega::routes(v6Server);V61Bundle::routes(v6Server);
 V62Modules::routes(v6Server);V62ScriptApps::routes(v6Server);V62Apps::routes(v6Server);
 v62Register();WolfDeck::bootStep(lcd,95,"ALL PROGRAMS REGISTERED");
 // Retain the existing manual-only SD mount policy.
 Serial.printf("WOLF DECK V6.2 touch=%s\n",touchOK?"YES":"NO");
 WolfDeck::bootReady(lcd);draw();
}
void loop(){
 v6Server.handleClient();V61WiFi::tick();V61Bundle81100::tick();V61BleTransfer::tick();V62Scripts::tick();
 uint16_t x=0,y=0;bool pressed=readTouch(x,y);
 if(pressed&&page==TOUCH&&y<185)lcd.fillCircle(x,y,3,BLUE);
 if(page==WIFI_KEYBOARD){
  int event=wifiKeys.update(pressed,V62Keys::hit(x,y),millis());
  if(!pressed&&wifiActiveKey>=0)wifiKeyHighlight(-1);
  if(event>=0)wifiKeyboardKey(x,y);
 }else if(libraryScroll(pressed,x,y)||v62ScrollTouch(pressed,x,y)){
  // Dedicated drag path; never activate an app under the scrollbar.
 }else if(pressed&&!lastPressed&&millis()-lastTouch>220){
  lastTouch=millis();
  if(page==BADUSB_FILES||page==UART_FILES||page==SCRIPT_INFO){libraryTouch(x,y);}
  else if(page==PROGRAMS||page==PROGRAM_ACTION||page==PROGRAM_RESULT){v62Touch(x,y);}
  else if(page==HOME){
   int index=V62Menu::homeIndex(x,y);
   if(index>=0)enter(homeItems[index],HOME);
  }else if(page==WIFI_SETUP){
    if(y>=198&&y<233&&x>=8&&x<112){goBack();}
    else if(y>=55&&y<130){
      wifiField=(y-55)/19;
      if(wifiField>3)wifiField=3;
      String *fields[]={&wifiEditSSID,&wifiEditPass,&wifiEditApSSID,&wifiEditApPass};
      wifiReturnPage=WIFI_SETUP;wifiOpenKeyboard(*fields[wifiField]);
    }else if(y>=151&&y<190){
      if(x<156){
        if(V61WiFi::validSSID(wifiEditSSID)&&(wifiEditPass.isEmpty()||V61WiFi::validPass(wifiEditPass))){
          bool ok=V61WiFi::upsert(wifiEditSSID,wifiEditPass,10);v6Log(ok?"STA profile saved":"STA profile save failed");
        }else v6Log("STA fields invalid");
      }else{
        if(V61WiFi::setAP(wifiEditApSSID,wifiEditApPass)){
          v6SSID=V61WiFi::apSSID;v6Password=V61WiFi::apPassword;v6Log("AP settings saved");
        }else v6Log("AP fields invalid");
      }draw();
    }
  }else if(page==SETTINGS&&y>=190&&x>=120){
    wifiEditApSSID=V61WiFi::apSSID;wifiEditApPass="";
    enter(WIFI_SETUP,SETTINGS);
  }else if(y>=190&&x<125){goBack();}
  else if(page==RADIO||page==HARDWARE||page==TOOLS){
   const Page *items=page==RADIO?radioItems:page==HARDWARE?hwItems:toolItems;
   int count=page==RADIO?4:6;
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
 v62SyncBle();
 if(page==SCRIPT_INFO&&millis()-lastSystemDraw>250){lastSystemDraw=millis();lcd.fillRect(0,190,300,8,PANEL);label(clipped(String(V62Scripts::running?"RUN ":"IDLE ")+String(V62Scripts::step)+"/"+String(V62Scripts::program.steps.size())+" "+V62Scripts::message,48),8,190,1,V62Scripts::running?GREEN:MUTED);}
 if(page==SYSTEM&&millis()-lastSystemDraw>2500){lastSystemDraw=millis();draw();}
 delay(page==WIFI_KEYBOARD?10:25);
}
