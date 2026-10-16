#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ES3C28P documented onboard wiring. Do not reuse old board's GPIO assignments.
static constexpr int LCD_CS=10, LCD_DC=46, LCD_SCK=12, LCD_MOSI=11, LCD_MISO=13, LCD_BL=45;
static constexpr int TOUCH_SDA=16, TOUCH_SCL=15, TOUCH_RST=18, TOUCH_ADDR=0x38;
SPIClass lcdSPI(FSPI);
Adafruit_ILI9341 lcd(&lcdSPI, LCD_DC, LCD_CS, -1);
static constexpr uint16_t BG=0x0841, PANEL=0x18E3, ACCENT=0x07FF, WHITE=0xFFFF, MUTED=0xBDF7;
int page=0; bool touchOK=false; bool lastPressed=false;
uint32_t lastTouch=0;

bool readTouch(uint16_t &x, uint16_t &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02); // TD_STATUS
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(TOUCH_ADDR, (uint8_t)5)!=(uint8_t)5) return false;
  uint8_t s=Wire.read(), xh=Wire.read(), xl=Wire.read(), yh=Wire.read(), yl=Wire.read();
  if((s&0x0F)==0 || (s&0x0F)>2) return false;
  uint16_t rawX=((xh&0x0F)<<8)|xl, rawY=((yh&0x0F)<<8)|yl;
  // LCD rotation(1) = landscape. Touch orientation may need calibration.
  x=constrain((int)rawY,0,319);
  y=constrain(239-(int)rawX,0,239);
  return true;
}
void label(const String &s,int x,int y,uint8_t sz=2,uint16_t color=WHITE){lcd.setTextSize(sz);lcd.setTextColor(color);lcd.setCursor(x,y);lcd.print(s);}
void button(int x,int y,int w,int h,const String &s){lcd.fillRoundRect(x,y,w,h,9,PANEL);lcd.drawRoundRect(x,y,w,h,9,ACCENT);label(s,x+12,y+15,2);}
void header(const String &s){lcd.fillScreen(BG);lcd.fillRect(0,0,320,37,PANEL);label(s,12,10,2,ACCENT);}
void draw(){
 if(page==0){header("CYBERDECK V4.6");button(10,49,145,55,"SYSTEM");button(165,49,145,55,"TOUCH");button(10,115,145,55,"WIFI");button(165,115,145,55,"MODULES");label("ES3C28P | TEST BUILD",12,205,1,MUTED);}
 else if(page==1){header("SYSTEM INFO");label("ESP32-S3",12,55);label("Flash: "+String(ESP.getFlashChipSize()/1048576)+" MB",12,90,2);label("PSRAM: "+String(ESP.getPsramSize()/1048576)+" MB",12,120,2);label("Heap: "+String(ESP.getFreeHeap()/1024)+" KB",12,150,2);button(10,185,120,45,"BACK");}
 else if(page==2){header("TOUCH TEST");label("Touch screen to draw",12,50,2);button(10,185,120,45,"BACK");}
 else if(page==3){header("WIFI STATUS");label("MAC available via Serial",12,58,1);label("WiFi scan coming next",12,92,2);button(10,185,120,45,"BACK");}
 else {header("MODULES");label("NFC / CC1101",12,58,2);label("Not enabled in test build",12,92,1);button(10,185,120,45,"BACK");}
}
void setup(){
 Serial.begin(115200);delay(300);
 pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,HIGH);
 lcdSPI.begin(LCD_SCK,LCD_MISO,LCD_MOSI,LCD_CS);
 lcd.begin(40000000);lcd.setRotation(1);
 pinMode(TOUCH_RST,OUTPUT);digitalWrite(TOUCH_RST,LOW);delay(15);digitalWrite(TOUCH_RST,HIGH);delay(120);
 Wire.begin(TOUCH_SDA,TOUCH_SCL,100000);
 Wire.beginTransmission(TOUCH_ADDR);touchOK=Wire.endTransmission()==0;
 Serial.printf("ES3C28P Touch at 0x38: %s; Flash %u; PSRAM %u\n",touchOK?"YES":"NO",ESP.getFlashChipSize(),ESP.getPsramSize());
 draw();
}
void loop(){
 uint16_t x=0,y=0;bool pressed=readTouch(x,y);
 if(pressed && page==2 && y<180){lcd.fillCircle(x,y,3,ILI9341_YELLOW);}
 if(pressed && !lastPressed && millis()-lastTouch>180){
   lastTouch=millis();Serial.printf("Touch x=%u y=%u\n",x,y);
   if(page==0){if(y>=49&&y<104)page=x<160?1:2;else if(y>=115&&y<170)page=x<160?3:4;draw();}
   else if(y>=185&&x<135){page=0;draw();}
 }
 lastPressed=pressed;delay(20);
}
