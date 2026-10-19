#pragma once
#include <cstring>
namespace V62Menu {
enum Category { Network, Bluetooth, Hardware, Files, System, Settings, Modules, Count };
static constexpr int All=-1;
static const char *const labels[]={"NETWORK","BLUETOOTH","HARDWARE","FILES / SCRIPTS","SYSTEM","SETTINGS","RADIO / MODULES"};
static const char *const keys[]={"network","bluetooth","hardware","files","system","settings","modules"};
inline bool has(const char *p,const char *part){return std::strstr(p,part)!=nullptr;}
inline int category(const char *p){
 if(has(p,"/wifi/ap")||has(p,"/wifi/networks")||has(p,"/wifi/delete"))return Settings;
 if(has(p,"ble")||has(p,"bluetooth"))return Bluetooth;
 if(has(p,"scripts")||has(p,"/sd/")||has(p,"storage"))return Files;
 if(has(p,"uart")||has(p,"adc")||has(p,"i2c")||has(p,"pins")||has(p,"hardware/plan")||has(p,"usb-hid"))return Hardware;
 if(has(p,"wifi")||has(p,"network")||has(p,"/scan/")||has(p,"/history"))return Network;
 if(has(p,"nfc")||has(p,"cc1101")||has(p,"subghz")||has(p,"/ir/"))return Modules;
 return System;
}
// Same rectangles are used by drawing and touch hit testing.
static constexpr int HomeX=8,HomeY=40,HomeDX=156,HomeDY=45,HomeW=148,HomeH=40;
inline int homeIndex(int x,int y){
 if(x<HomeX||y<HomeY)return -1;
 int col=(x-HomeX)/HomeDX,row=(y-HomeY)/HomeDY;
 if(col>1||row>3||(x-HomeX)%HomeDX>=HomeW||(y-HomeY)%HomeDY>=HomeH)return -1;
 return row*2+col;
}
}
