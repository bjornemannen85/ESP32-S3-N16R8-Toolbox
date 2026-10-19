#include "v62_wolfdeck.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
struct LCD {
 int writes=0,starts=0,ends=0,x=0,y=0,w=0,h=0;
 std::vector<uint16_t> pixels;
 std::vector<std::string> labels;
 void startWrite(){starts++;pixels.clear();}
 void endWrite(){ends++;}
 void setAddrWindow(int a,int b,int c,int d){x=a;y=b;w=c;h=d;}
 void writePixels(uint16_t *p,int n,bool block,bool bigEndian){assert(block&&!bigEndian);writes++;pixels.insert(pixels.end(),p,p+n);}
 void fillRoundRect(int,int,int,int,int,uint16_t){}
 void drawRoundRect(int,int,int,int,int,uint16_t){}
 void fillRect(int x,int y,int w,int h,uint16_t){assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=320&&y+h<=240);}
 void setTextColor(uint16_t){}
 void setTextSize(int){}
 void setCursor(int,int){}
 void print(const char *s){labels.emplace_back(s);}
};
int main(){
 LCD lcd;
 static_assert(sizeof(WolfDeck::row)==640);
 WolfDeck::background(lcd);assert(lcd.pixels.size()==320*240&&lcd.writes==240);
 for(size_t i=0;i<lcd.pixels.size();i++)assert(lcd.pixels[i]==WolfDeck::dim565(WolfDeckAssets::displayPalette[WolfDeckAssets::displayPixels[i]],1));
 WolfDeck::panel(lcd,5,39,73,54);assert(lcd.pixels.size()==73*54);
 for(int y=0;y<54;y++)for(int x=0;x<73;x++)assert(lcd.pixels[y*73+x]==WolfDeck::dim565(WolfDeckAssets::displayPalette[WolfDeckAssets::displayPixels[(39+y)*320+5+x]],2));
 int starts=lcd.starts;WolfDeck::panel(lcd,-1,0,10,10);WolfDeck::panel(lcd,315,0,10,10);assert(lcd.starts==starts);
 WolfDeck::bootBegin(lcd);assert(WolfDeck::booting&&WolfDeck::progress==0&&lcd.pixels.size()==76800);
 WolfDeck::bootStep(lcd,60,"WIFI CONFIG LOADED");assert(WolfDeck::progress==60);
 WolfDeck::bootStep(lcd,40,"NO REGRESSION");assert(WolfDeck::progress==60);
 WolfDeck::bootReady(lcd);assert(WolfDeck::progress==100&&!WolfDeck::booting);
 assert(lcd.labels.back()=="READY");
 bool version=false;for(const auto &s:lcd.labels)if(s=="V6.2")version=true;assert(version);assert(testDelay>=5000&&testDelay<=5200);assert(lcd.starts==lcd.ends);
 auto count=lcd.labels.size();WolfDeck::bootStep(lcd,50,"LATE");assert(lcd.labels.size()==count);
 std::cout<<"PASS: flash palettes, scanline bounds, 640-byte buffer, background/panels, monotonic boot stages, logo frames, READY, bounded timing\n";
}
