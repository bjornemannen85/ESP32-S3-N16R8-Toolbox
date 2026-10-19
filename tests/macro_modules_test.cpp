#include "v62_macro_core.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
int main(int argc,char **argv){
 using namespace V62ModuleCore;
 for(int p:{0,3,10,11,12,13,15,16,18,19,20,26,33,37,38,45,48,49})assert(!pinAllowed(p));
 for(int p:{1,2,4,5,6,7,8,9,14,17,21,42})assert(pinAllowed(p));
 std::vector<int> pinsOut;assert(pins("8,9,4,5",pinsOut,4));assert(!pins("8,8",pinsOut,2));assert(!pins("8,19",pinsOut,2));assert(!pins("8,9,",pinsOut,2));assert(!pins("8x,9",pinsOut,2));
 std::vector<uint8_t> bytes;assert(hex("00 FF 10 EF",bytes,4)&&bytes.size()==4);assert(!hex("ABC",bytes,4));assert(!hex("00 FF",bytes,1));
 uint8_t f[3];assert(frequency(433920000,f));assert(f[0]==0x10&&f[1]==0xb0&&f[2]==0x71);assert(!frequency(500000000,f));
 uint8_t rom[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0f};assert(crc8(rom,7)==rom[7]);
 auto p=V62Macro::parse("REM test\nTX H\\r\\n\nDELAY 20\nTXHEX 00 FF\nREPEAT 2\nWAIT_RX 200\n",true);assert(p.error.empty()&&p.steps.size()==6);assert(V62Macro::unescape("x\\r\\n")=="x\r\n");
 assert(!V62Macro::parse("STRING \xc3\xb8\n",false).error.empty());
 assert(!V62Macro::parse("REPEAT 2\n",true).error.empty());assert(!V62Macro::parse("TXHEX GG\n",true).error.empty());assert(!V62Macro::parse("DELAY -1\n",false).error.empty());assert(!V62Macro::parse("UNKNOWN\n",false).error.empty());
 assert(V62Macro::parse("ID 1234:abcd Test\nGUI r\nSTRING hello\nENTER\n",false).error.empty());
 for(int i=1;i<argc;i++){std::ifstream input(argv[i]);assert(input);std::string text((std::istreambuf_iterator<char>(input)),{});bool uart=std::string(argv[i]).find(".uart")!=std::string::npos;auto result=V62Macro::parse(text,uart);if(!result.error.empty())std::cerr<<argv[i]<<":"<<result.errorLine<<" "<<result.error<<"\n";assert(result.error.empty());}
 std::cout<<"PASS: pin ownership policy, numeric/hex validation, CC1101 frequency, Dallas CRC, UART/Ducky parsing and all bundled runnable scripts\n";
}
