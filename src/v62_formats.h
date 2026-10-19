#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "v62_macro_core.h"
namespace V62Formats {
struct Item{std::string key,value;};
struct Result{std::string type,error;std::vector<Item> fields;size_t rawPulses=0;};
inline Result inspect(const std::string &body){
 Result r;if(body.size()>8192){r.error="8 KiB text file limit";return r;}std::istringstream lines(body);std::string line;
 while(std::getline(lines,line)){
  line=V62Macro::trim(line);if(line.empty()||line[0]=='#')continue;size_t colon=line.find(':');if(colon==std::string::npos){r.error="Expected Key: value";return r;}
  auto key=V62Macro::trim(line.substr(0,colon)),value=V62Macro::trim(line.substr(colon+1));
  if(key=="Filetype")r.type=value;
  if(key=="RAW_Data"||key=="data"){
   std::istringstream values(value);std::string v;long n;while(values>>v){if(!V62ModuleCore::integer(v,n,-1000000,1000000)||n==0){r.error="Invalid raw timing";return r;}r.rawPulses++;}
  }else if(r.fields.size()<64&&key.size()<=48&&value.size()<=256)r.fields.push_back({key,value});
 }
 const char *types[]={"Flipper SubGhz RAW File","Flipper SubGhz Key File","IR signals file","Flipper NFC device","Flipper RFID key","Flipper iButton key"};bool recognized=false;for(auto type:types)if(r.type==type)recognized=true;
 if(!recognized)r.error="Unrecognized Flipper text file type";
 return r;
}
}
