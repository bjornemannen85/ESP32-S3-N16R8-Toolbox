#include "v62_keyboard.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace V62Keys;
 for(int row=0;row<4;row++)for(int col=0;col<10;col++){
  assert(hit(col*32+16,45+row*34+16)==row*10+col);
 }
 assert(hit(16,193)==40);assert(hit(100,193)==41);assert(hit(180,193)==42);
 assert(hit(180,215)==44);assert(hit(280,193)==43);
 assert(hit(0,34)==-1);assert(hit(320,100)==-1);assert(hit(1,233)==-1);
 Input input;
 assert(input.update(true,0,1000)==-1); // Opening finger cannot type a stray key.
 assert(input.update(false,-1,1010)==-1);
 assert(input.update(true,0,1050)==0);
 assert(input.update(true,0,1100)==-1); // Holding does not repeat.
 input.update(false,-1,1110);
 assert(input.update(true,0,1140)==0); // Repeated letter, well inside old 220 ms limit.
 input.update(false,-1,1150);
 assert(input.update(true,1,1190)==1); // Rapid different letter.
 assert(input.update(true,2,1240)==2); // Moving onto another key also works.
 input.update(false,-1,1241);
 assert(input.update(true,2,1245)==-1); // Suppress contact bounce.
 assert(input.update(true,2,1280)==2);
 input.reset();assert(input.update(true,30,1400)==-1);
 input.update(false,-1,1410);assert(input.update(true,30,1450)==30);
 std::cout<<"PASS: all key centers/controls, short taps, repeated letters, held keys, movement, bounce, opening release\n";
}
