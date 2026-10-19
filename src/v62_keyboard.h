#pragma once
namespace V62Keys {
inline int hit(int x,int y){
 if(x<0||x>=320)return -1;
 if(y>=45&&y<181)return ((y-45)/34)*10+x/32;
 if(y>=186&&y<225){
  if(x<80)return 40;
  if(x<160)return 41;
  if(x<240)return y>=208?44:42;
  return 43;
 }
 return -1;
}
struct Input {
 bool waitRelease=true;
 int held=-1;
 unsigned long lastEvent=0;
 void reset(){waitRelease=true;held=-1;lastEvent=0;}
 int update(bool down,int key,unsigned long now){
  if(waitRelease){if(!down)waitRelease=false;return -1;}
  if(!down){held=-1;return -1;}
  if(key<0||key==held||now-lastEvent<35)return -1;
  held=key;lastEvent=now;return key;
 }
};
}
