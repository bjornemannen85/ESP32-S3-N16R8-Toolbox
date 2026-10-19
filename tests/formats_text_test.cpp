#include "v62_formats.h"
#include "v62_text.h"
#include <cassert>
#include <iostream>
int main(){
 auto result=V62Formats::inspect("Filetype: Flipper SubGhz RAW File\nVersion: 1\nFrequency: 433920000\nRAW_Data: 100 -200 300 -400\n");assert(result.error.empty()&&result.rawPulses==4);
 assert(!V62Formats::inspect("Filetype: unsupported\n").error.empty());
 assert(!V62Formats::inspect("Filetype: IR signals file\ndata: 100 0 200\n").error.empty());
 assert(V62Formats::inspect("Filetype: Flipper RFID key\nVersion: 1\nKey type: EM4100\nData: 01 02 03 04 05\n").error.empty());
 std::string text(49152,'\n');assert(V62Text::lines(text.data(),text.size())==49152);assert(V62Text::offset(text.data(),text.size(),49135)==49135);
 text=std::string(49,'a')+"\r\nb\n";assert(V62Text::lines(text.data(),text.size())==3);assert(V62Text::offset(text.data(),text.size(),1)==48);assert(V62Text::line(text.data(),text.size(),51)==2);
 std::cout<<"PASS: Flipper format metadata/raw validation and constant-memory text scrolling through 48 KiB results\n";
}
