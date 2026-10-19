#include "v62_icons.h"
#include <cassert>
#include <cstring>
#include <iostream>
struct Display {
 const uint8_t *bits=nullptr;int x=0,y=0,w=0,h=0;uint16_t color=0;
 void drawBitmap(int px,int py,const uint8_t *p,int width,int height,uint16_t c){x=px;y=py;bits=p;w=width;h=height;color=c;}
};
int main(){
 assert(V62Icons::Count==41);
 assert(sizeof(V62Icons::routes)/sizeof(V62Icons::routes[0])>=113);
 assert(sizeof(V62Icons::pages)==32);
 for(const auto &icon:V62Icons::icons){
  assert(icon.bits&&icon.color&&icon.length==std::strlen(icon.svg));
  assert(std::strstr(icon.svg,"<svg")&&std::strstr(icon.svg,"viewBox=\"0 0 64 64\""));
  bool nonempty=false;for(int i=0;i<72;i++)nonempty|=icon.bits[i]!=0;assert(nonempty);
  Display display;V62Icons::draw(display,icon,8,53);
  assert(display.x==8&&display.y==53&&display.w==24&&display.h==24&&display.bits==icon.bits&&display.color==icon.color);
  assert(&V62Icons::get(icon.id)==&icon);
 }
 for(const auto &route:V62Icons::routes){assert(route.icon<V62Icons::Count);assert(&V62Icons::route(route.path)==&V62Icons::icons[route.icon]);}
 assert(std::strcmp(V62Icons::route("/api/v61/uart/start").id,"uart")==0);
 assert(std::strcmp(V62Icons::route("/wifi/nat").id,"nat")==0);
 assert(std::strcmp(V62Icons::route("/scripts/save").id,"scripts")==0);
 assert(std::strcmp(V62Icons::categoriesIcon(-1).id,"all-tools")==0);
 assert(std::strcmp(V62Icons::homeIcon(7).id,"all-tools")==0);
 assert(std::strcmp(V62Icons::pagesIcon(19).id,"wifi-setup")==0);
 assert(std::strcmp(V62Icons::route(nullptr).id,"system")==0);
 assert(std::strcmp(V62Icons::get("../invalid").id,"system")==0);
 assert(std::strcmp(V62Icons::pagesIcon(999).id,"system")==0);
 std::cout<<"PASS: 41 flash SVGs, 24px masks, 113+ route mappings, native pages/categories, bounded draw calls and fallbacks\n";
}
