#pragma once
#include <cstddef>
#include <cstdint>
#include <algorithm>
namespace V62Scroll {
struct Bar {
 static constexpr int X=302,W=18,Y=51,H=139;
 size_t total=0,visible=1,offset=0;
 size_t limit()const{return total>visible?total-visible:0;}
 int thumbHeight()const{return limit()?std::max(18,int(H*visible/total)):H;}
 int thumbY()const{return Y+(limit()?int((H-thumbHeight())*offset/limit()):0);}
 bool hit(int x,int y)const{return x>=X&&x<320&&y>=Y&&y<Y+H;}
 size_t seek(int y)const{int travel=H-thumbHeight();if(travel<=0)return 0;int pos=std::max(0,std::min(travel,y-Y-thumbHeight()/2));return (size_t(pos)*limit()+travel/2)/travel;}
 template<class LCD>void draw(LCD &lcd,uint16_t track,uint16_t thumb)const{
  lcd.fillRoundRect(X+4,Y,10,H,4,track);lcd.fillRoundRect(X+4,thumbY(),10,thumbHeight(),4,thumb);
 }
};
}
