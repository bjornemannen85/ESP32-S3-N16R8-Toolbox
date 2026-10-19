#include <cstdint>
#include "v62_scroll.h"
#include <cassert>
#include <iostream>
int main(){
 for(size_t total: {0u,1u,5u,6u,92u,1024u}){
  V62Scroll::Bar b{total,5,0};assert(b.thumbHeight()>=18&&b.thumbHeight()<=139);
  assert(b.seek(-100)==0&&b.seek(1000)==b.limit());assert(!b.hit(301,100)&&b.hit(302,51)&&!b.hit(320,100)&&!b.hit(309,190));
  size_t last=0;for(int y=51;y<190;y++){auto n=b.seek(y);assert(n>=last&&n<=b.limit());last=n;}
  b.offset=b.limit();assert(b.thumbY()+b.thumbHeight()==190);
 }
 std::cout<<"PASS: scrollbar empty/short/long lists, endpoint reachability, monotonic drag and reserved hit area\n";
}
