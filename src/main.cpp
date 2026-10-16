#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
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

enum Page { HOME, RADIO, HARDWARE, TOOLS, WIFI, BLE, SYSTEM, TOUCH, NFC, SUBGHZ, IR, PAGE_GPIO, UART, I2C, SPI_DIAG, STORAGE, SETTINGS, LOGS, MODULES };
Page page=HOME, parent=HOME;
void draw();
Page history[12];
int historyDepth=0;

void goBack(){
  if(historyDepth>0){
    page=history[--historyDepth];
  }else{
    page=HOME;
  }
  draw();
}
bool lastPressed=false,touchOK=false,bleReady=false;
uint32_t lastTouch=0,lastSystemDraw=0;
String wifiLines[7],bleLines[7]; int wifiCount=-1,bleCount=-1;
const char *names[]={"HOME","RADIO TOOLS","HARDWARE","UTILITIES","WIFI SCANNER","BLE SCANNER","SYSTEM MONITOR","TOUCH TEST","PN532 NFC / EMV","CC1101 SUB-GHZ","IR ANALYZER","GPIO LAB","UART TERMINAL","I2C EXPLORER","SPI DIAGNOSTICS","MICROSD STORAGE","SETTINGS","EVENT LOG","MODULE MANAGER"};
const Page radioItems[]={WIFI,BLE,NFC,SUBGHZ,IR};
const Page hwItems[]={PAGE_GPIO,UART,I2C,SPI_DIAG,STORAGE,MODULES};
const Page toolItems[]={SYSTEM,TOUCH,SETTINGS,LOGS};

bool readTouch(uint16_t &x,uint16_t &y){
  Wire.beginTransmission(TOUCH_ADDR);Wire.write(0x02);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(TOUCH_ADDR,(uint8_t)5)!=(uint8_t)5)return false;
  uint8_t s=Wire.read(),xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();
  if((s&0x0F)==0 || (s&0x0F)>2)return false;
  uint16_t rx=((xh&0x0F)<<8)|xl,ry=((yh&0x0F)<<8)|yl;
  x=constrain((int)ry,0,319);y=constrain(239-(int)rx,0,239);return true;
}
void label(const String &s,int x,int y,uint8_t sz=2,uint16_t color=WHITE){lcd.setTextSize(sz);lcd.setTextColor(color);lcd.setCursor(x,y);lcd.print(s);}
uint16_t tileColor(Page p){switch(p){case WIFI:case RADIO:return ACCENT;case BLE:return BLUE;case NFC:return PURPLE;case SUBGHZ:return ORANGE;case IR:return RED;case PAGE_GPIO:return GREEN;case UART:return ACCENT;case I2C:case SPI_DIAG:return YELLOW;case SYSTEM:return BLUE;case STORAGE:return BLUE;default:return MUTED;}}
String clipped(String s,size_t maxLen);
void tile(int x,int y,int w,int h,const String &title,Page target){
 uint16_t c=tileColor(target);
 lcd.fillRoundRect(x,y,w,h,7,PANEL);lcd.drawRoundRect(x,y,w,h,7,c);
 lcd.fillRoundRect(x+7,y+7,27,27,5,BG);lcd.drawRoundRect(x+7,y+7,27,27,5,c);
 // Simple hardware-rendered pictogram: no external fonts or image assets.
 if(target==WIFI || target==RADIO){
   lcd.drawCircle(x+20,y+20,8,c);
   lcd.fillCircle(x+20,y+20,2,c);
 }else if(target==BLE){
   lcd.drawLine(x+20,y+11,x+20,y+29,c);
   lcd.drawLine(x+20,y+11,x+26,y+17,c);
   lcd.drawLine(x+26,y+17,x+15,y+25,c);
   lcd.drawLine(x+15,y+15,x+26,y+25,c);
   lcd.drawLine(x+26,y+25,x+20,y+29,c);
 }else if(target==SYSTEM || target==TOOLS){
   lcd.drawRoundRect(x+13,y+13,15,15,3,c);
   lcd.drawLine(x+16,y+24,x+20,y+17,c);
   lcd.drawLine(x+20,y+17,x+25,y+24,c);
 }else{
   lcd.drawRect(x+14,y+14,13,13,c);
   lcd.fillCircle(x+20,y+20,2,c);
 }
 label(clipped(title,17),x+41,y+13,1,WHITE);
}
void button(int x,int y,int w,int h,const String &s){lcd.fillRoundRect(x,y,w,h,7,PANEL);lcd.drawRoundRect(x,y,w,h,7,ACCENT);label(s,x+10,y+(h-16)/2,2);}

void header(const String &s){lcd.fillScreen(BG);lcd.fillRect(0,0,320,33,PANEL);lcd.drawFastHLine(0,33,320,ACCENT);label(clipped(s,25),8,9,1,ACCENT);}
void backButton(){button(8,198,104,35,"BACK");}
void footer(const String &s){label(s,9,235,1,MUTED);}
String clipped(String s,size_t n){if(s.length()>n)s=s.substring(0,n-2)+"..";return s;}
void menu(const Page *items,int count){
 for(int i=0;i<count;i++){int col=i%2,row=i/2;tile(8+col*156,42+row*49,148,43,names[items[i]],items[i]);}
 backButton();
}
void draw(){
 header(page==HOME?"CYBERDECK V4.9 PRO":names[page]);
 switch(page){
 case HOME:
  tile(8,43,148,45,"RADIO",RADIO);tile(164,43,148,45,"HARDWARE",HARDWARE);
  tile(8,94,148,45,"TOOLS",TOOLS);tile(164,94,148,45,"SYSTEM",SYSTEM);
  tile(8,145,148,45,"WIFI",WIFI);tile(164,145,148,45,"BLE",BLE);
  footer("V4.9 PRO  |  ES3C28P  |  SAFE");break;
 case RADIO: menu(radioItems,5);break;
 case HARDWARE: menu(hwItems,6);break;
 case TOOLS: menu(toolItems,4);break;
 case SYSTEM:
  label("Flash: "+String(ESP.getFlashChipSize()/1048576)+" MB",10,48);
  label("PSRAM: "+String(ESP.getPsramSize()/1048576)+" MB",10,76);
  label("Heap: "+String(ESP.getFreeHeap()/1024)+" KB",10,104);
  label("Uptime: "+String(millis()/1000)+" s",10,132);
  label("CPU: "+String(ESP.getCpuFreqMHz())+" MHz",10,161,1,MUTED);backButton();break;
 case TOUCH:label("Draw on screen",10,52);backButton();break;
 case WIFI:
  label(wifiCount<0?"Press SCAN":String("Networks: ")+wifiCount,10,43,1,MUTED);
  for(int i=0;i<7;i++)if(wifiLines[i].length())label(wifiLines[i],10,61+i*18,1);
  backButton();button(174,195,138,37,"SCAN");break;
 case BLE:
  label(bleCount<0?"Press SCAN":String("Devices: ")+bleCount,10,43,1,MUTED);
  for(int i=0;i<7;i++)if(bleLines[i].length())label(bleLines[i],10,61+i*18,1);
  backButton();button(174,195,138,37,"SCAN");break;
 case MODULES:
  label("PN532: NOT CONFIGURED",8,52,1,ORANGE);
  label("CC1101: NOT CONFIGURED",8,76,1,ORANGE);
  label("IR: NOT CONFIGURED",8,100,1,ORANGE);
  label("SD: NOT MOUNTED",8,124,1,ORANGE);
  label("GPIO: NOT CONFIGURED",8,155,1,MUTED);backButton();break;
 case NFC:case SUBGHZ:case IR:case PAGE_GPIO:case UART:case I2C:case SPI_DIAG:case STORAGE:
  label("NOT CONFIGURED",12,65,2,ORANGE);
  label("Hardware / pins pending",12,103,1,MUTED);
  label("GPIO outputs disabled",12,122,1,MUTED);backButton();break;
 case SETTINGS:
  label("Pin mapping: pending",12,57,1,ORANGE);
  label("WiFi setup: pending",12,79,1,ORANGE);
  label("Saved configs: pending",12,101,1,ORANGE);backButton();break;
 case LOGS:
  label("Runtime event log",12,58,1);
  label("Boot OK",12,80,1,MUTED);
  label(touchOK?"Touch OK":"Touch unavailable",12,102,1,MUTED);backButton();break;
 }
}
void scanWifi(){header("WIFI SCANNING...");for(auto &s:wifiLines)s="";WiFi.mode(WIFI_STA);WiFi.disconnect(false,false);delay(100);int n=WiFi.scanNetworks(false,true);wifiCount=n<0?0:n;for(int i=0;i<n&&i<7;i++){String ssid=WiFi.SSID(i);if(!ssid.length())ssid="<hidden>";wifiLines[i]=clipped(ssid,20)+" "+String(WiFi.RSSI(i))+"dBm ch"+String(WiFi.channel(i));}WiFi.scanDelete();draw();}
void scanBle(){header("BLE SCANNING...");for(auto &s:bleLines)s="";if(!bleReady){BLEDevice::init("");bleReady=true;}BLEScan *s=BLEDevice::getScan();s->setActiveScan(false);BLEScanResults results=s->start(4,false);bleCount=results.getCount();for(int i=0;i<bleCount&&i<7;i++){BLEAdvertisedDevice d=results.getDevice(i);String name=d.haveName()?String(d.getName().c_str()):String(d.getAddress().toString().c_str());bleLines[i]=clipped(name,24)+" "+String(d.getRSSI())+"dBm";}s->clearResults();draw();}
void enter(Page p,Page from){
  if(historyDepth<12) history[historyDepth++]=page;
  parent=from;
  page=p;
  draw();
}
void setup(){Serial.begin(115200);delay(250);pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,HIGH);lcdSPI.begin(LCD_SCK,LCD_MISO,LCD_MOSI,LCD_CS);lcd.begin(40000000);lcd.setRotation(1);pinMode(TOUCH_RST,OUTPUT);digitalWrite(TOUCH_RST,LOW);delay(15);digitalWrite(TOUCH_RST,HIGH);delay(120);Wire.begin(TOUCH_SDA,TOUCH_SCL,100000);Wire.beginTransmission(TOUCH_ADDR);touchOK=Wire.endTransmission()==0;WiFi.mode(WIFI_OFF);Serial.printf("ES3C28P V4.8 touch=%s\n",touchOK?"YES":"NO");draw();}
void loop(){
 uint16_t x=0,y=0;bool pressed=readTouch(x,y);
 if(pressed&&page==TOUCH&&y<185)lcd.fillCircle(x,y,3,BLUE);
 if(pressed&&!lastPressed&&millis()-lastTouch>220){
  lastTouch=millis();
  if(page==HOME){
   if(y>=43&&y<88)enter(x<160?RADIO:HARDWARE,HOME);
   else if(y>=94&&y<139)enter(x<160?TOOLS:SYSTEM,HOME);
   else if(y>=145&&y<190)enter(x<160?WIFI:BLE,HOME);
  }else if(y>=190&&x<125){goBack();}
  else if(page==RADIO||page==HARDWARE||page==TOOLS){
   const Page *items=page==RADIO?radioItems:page==HARDWARE?hwItems:toolItems;
   int count=page==RADIO?5:page==HARDWARE?6:4;
   if(y>=42&&y<183){int row=(int(y)-42)/49,col=x<160?0:1,index=row*2+col;
    if(index>=0&&index<count&&y<42+row*49+43)enter(items[index],page);
   }
  }else if(y>=195&&x>=174&&page==WIFI)scanWifi();
  else if(y>=195&&x>=174&&page==BLE)scanBle();
 }
 lastPressed=pressed;
 if(page==SYSTEM&&millis()-lastSystemDraw>2500){lastSystemDraw=millis();draw();}
 delay(25);
}
