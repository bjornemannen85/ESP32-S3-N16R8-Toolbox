#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <cstdlib>
#include <cerrno>
namespace V62ModuleCore {
inline bool pinAllowed(int pin){
 if(pin<1||pin>48||(pin>=26&&pin<=37))return false;
 const int reserved[]={3,10,11,12,13,15,16,18,19,20,38,39,40,41,43,44,45,46,47,48};
 for(int p:reserved)if(pin==p)return false;
 return true;
}
inline bool integer(const std::string &s,long &value,long low,long high){
 if(s.empty()||s.size()>12)return false;
 char *end=nullptr;errno=0;long n=std::strtol(s.c_str(),&end,10);
 if(errno||end!=s.c_str()+s.size()||n<low||n>high)return false;
 value=n;return true;
}
inline bool pins(const std::string &text,std::vector<int> &out,size_t count){
 out.clear();size_t pos=0;
 while(pos<text.size()){
  size_t end=text.find(',',pos);if(end==std::string::npos)end=text.size();long p;
  if(!integer(text.substr(pos,end-pos),p,1,48)||!pinAllowed(p))return false;
  for(int old:out)if(old==p)return false;
  out.push_back(int(p));
  if(end==text.size())break;
  pos=end+1;if(pos==text.size())return false;
 }
 return out.size()==count;
}
inline uint8_t crc8(const uint8_t *data,size_t len){uint8_t crc=0;for(size_t i=0;i<len;i++){uint8_t v=data[i];for(int j=0;j<8;j++){uint8_t mix=(crc^v)&1;crc>>=1;if(mix)crc^=0x8c;v>>=1;}}return crc;}
inline bool hex(const std::string &s,std::vector<uint8_t> &out,size_t maximum){
 out.clear();int high=-1;for(char c:s){if(c==' '||c=='\t')continue;int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(n<0)return false;if(high<0)high=n;else{if(out.size()>=maximum)return false;out.push_back(uint8_t(high*16+n));high=-1;}}
 return high<0&&!out.empty();
}
// CC1101 synthesizer register bytes from the 26 MHz crystal.
inline bool frequency(uint32_t hz,uint8_t out[3]){
 if(!((hz>=300000000&&hz<=348000000)||(hz>=387000000&&hz<=464000000)||(hz>=779000000&&hz<=928000000)))return false;
 uint32_t word=uint32_t((uint64_t(hz)*65536+13000000)/26000000);out[0]=word>>16;out[1]=word>>8;out[2]=word;return true;
}
}
