#pragma once
#include "v62_wolfdeck_assets.h"

namespace WolfDeck {
// Single scanline, not a framebuffer; the palettes and pixels stay in flash.
static uint16_t row[320];
static unsigned progress=0;
static bool booting=false;
static unsigned long bootStarted=0;
static constexpr unsigned long BootDurationMs=5000,ReadyHoldMs=1000;
static uint16_t dim565(uint16_t c,unsigned shifts){
 while(shifts--)c=(c&0xf7deu)>>1;
 return c;
}
template<class LCD> static void region(LCD &lcd,const uint8_t *pixels,const uint16_t *palette,
 int x,int y,int w,int h,unsigned dim=0){
 if(x<0||y<0||w<=0||h<=0||x+w>320||y+h>240)return;
 lcd.startWrite();lcd.setAddrWindow(x,y,w,h);
 for(int yy=y;yy<y+h;yy++){
  for(int xx=0;xx<w;xx++){
   uint8_t index=pgm_read_byte(pixels+yy*320+x+xx);
   row[xx]=dim565(pgm_read_word(palette+index),dim);
  }
  lcd.writePixels(row,w,true,false);
 }
 lcd.endWrite();
}
template<class LCD> static void background(LCD &lcd){
 region(lcd,WolfDeckAssets::displayPixels,WolfDeckAssets::displayPalette,0,0,320,240,1);
}
template<class LCD> static void panel(LCD &lcd,int x,int y,int w,int h){
 region(lcd,WolfDeckAssets::displayPixels,WolfDeckAssets::displayPalette,x,y,w,h,2);
}
template<class LCD> static void typeLine(LCD &lcd,const char *text,int x,int y,int size,unsigned pause){
 char prefix[24]={0};
 for(unsigned i=0;text[i]&&i<sizeof(prefix)-1;i++){
  prefix[i]=text[i];prefix[i+1]=0;
  lcd.setTextSize(size);lcd.setTextColor(0x05ff);lcd.setCursor(x,y);lcd.print(prefix);delay(pause);
 }
}
template<class LCD> static void bootBegin(LCD &lcd){
 booting=true;progress=0;bootStarted=millis();
 region(lcd,WolfDeckAssets::bootPixels,WolfDeckAssets::bootPalette,0,0,320,240);
 lcd.fillRoundRect(68,3,184,31,5,0x0008);
 typeLine(lcd,"WOLF DECK",79,7,3,65);
 lcd.fillRoundRect(126,36,68,22,4,0x0008);
 typeLine(lcd,"V6.2",136,39,2,90);
 lcd.fillRoundRect(116,61,88,13,3,0x0008);
 lcd.setTextSize(1);lcd.setTextColor(0xb6bc);lcd.setCursor(127,64);lcd.print("FULL ACCESS");
 lcd.fillRoundRect(12,208,296,31,5,0x0008);
 lcd.drawRoundRect(16,211,288,10,3,0x05ff);
}
template<class LCD> static void bootStep(LCD &lcd,unsigned target,const char *stage){
 if(!booting)return;
 if(target>100)target=100;
 while(progress<target){
  ++progress;lcd.fillRect(19,214,(282*progress)/100,4,0x05ff);delay(2);
 }
 lcd.fillRect(16,225,288,12,0x0008);lcd.setTextSize(1);lcd.setTextColor(0xffff);
 lcd.setCursor(18,227);lcd.print(stage);
}
template<class LCD> static void bootReady(LCD &lcd){
 unsigned long elapsed=millis()-bootStarted;
 if(elapsed<BootDurationMs-ReadyHoldMs)delay(BootDurationMs-ReadyHoldMs-elapsed);
 bootStep(lcd,100,"READY");
 lcd.setTextColor(0x07e0);lcd.setCursor(18,227);lcd.print("READY");
 delay(ReadyHoldMs);booting=false;
}
}
