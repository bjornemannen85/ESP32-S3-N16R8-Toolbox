#pragma once
static size_t libraryOffset=0,librarySelected=0,libraryTextOffset=0;
static bool libraryGuide=true,libraryDrag=false;
static String libraryText;
static std::vector<size_t> libraryLines;
static bool libraryFolderUart=false;
static std::vector<size_t> libraryFiles(){std::vector<size_t> out;for(size_t i=0;i<V62ScriptAssets::Count;i++)if(String(V62ScriptAssets::files[i].folder)==(libraryFolderUart?"uart":"badusb"))out.push_back(i);return out;}
static void libraryIndex(){libraryLines.clear();size_t pos=0;while(pos<libraryText.length()){libraryLines.push_back(pos);int n=0;while(pos<libraryText.length()&&n<48){char c=libraryText[pos++];if(c=='\n')break;if(c!='\r')n++;}}}
static void libraryLoad(){const auto &f=V62ScriptAssets::files[librarySelected];libraryText=libraryGuide?String(f.guide):V62ScriptApps::content(f);libraryText.replace("ø","o");libraryText.replace("å","a");libraryText.replace("æ","ae");libraryText.replace("–","-");libraryIndex();libraryTextOffset=0;}
void libraryDraw(){
 if(page==BADUSB_FILES||page==UART_FILES){
  auto files=libraryFiles();label("Select file; instructions included",8,38,1,MUTED);
  for(size_t row=0;row<5&&libraryOffset+row<files.size();row++){const auto &f=V62ScriptAssets::files[files[libraryOffset+row]];int y=52+row*28;lcd.drawRoundRect(6,y,292,26,4,ACCENT);V62Icons::draw(lcd,V62Icons::get(libraryFolderUart?"uart":"usb"),8,y+1);label(clipped(f.name,42),38,y+8,1);}
  V62Scroll::Bar{files.size(),5,libraryOffset}.draw(lcd,PANEL,ACCENT);backButton();button(121,198,54,35,"<");button(181,198,54,35,">");button(241,198,72,35,"STOP");
 }else{
  const auto &f=V62ScriptAssets::files[librarySelected];label(clipped(f.name,48),8,38,1,ACCENT);
  for(size_t row=0;row<17&&libraryTextOffset+row<libraryLines.size();row++){size_t pos=libraryLines[libraryTextOffset+row];String line;while(pos<libraryText.length()&&line.length()<48){char c=libraryText[pos++];if(c=='\n')break;if(c!='\r')line+=c;}label(line,8,51+row*8,1);}
  V62Scroll::Bar{libraryLines.size(),17,libraryTextOffset}.draw(lcd,PANEL,ACCENT);
  label(clipped(String(V62Scripts::running?"RUN ":"IDLE ")+String(V62Scripts::step)+"/"+String(V62Scripts::program.steps.size())+" "+V62Scripts::message,48),8,190,1,V62Scripts::running?GREEN:MUTED);
  button(6,198,68,35,"BACK");button(78,198,68,35,libraryGuide?"CODE":"HELP");button(150,198,74,35,String(f.runtime)=="badusb"&&!V61USBHID::enabled?"ARM":"RUN");button(228,198,86,35,"STOP");
 }
}
bool libraryScroll(bool down,uint16_t x,uint16_t y){
 if(page!=BADUSB_FILES&&page!=UART_FILES&&page!=SCRIPT_INFO){libraryDrag=false;return false;}
 if(!down){bool was=libraryDrag;libraryDrag=false;return was;}
 auto files=libraryFiles();V62Scroll::Bar bar{page==SCRIPT_INFO?libraryLines.size():files.size(),size_t(page==SCRIPT_INFO?17:5),page==SCRIPT_INFO?libraryTextOffset:libraryOffset};
 if(!libraryDrag&&(!bar.hit(x,y)||lastPressed))return false;libraryDrag=true;size_t pos=bar.seek(y);if(pos!=bar.offset){if(page==SCRIPT_INFO)libraryTextOffset=pos;else libraryOffset=pos;draw();}return true;
}
void libraryTouch(uint16_t x,uint16_t y){
 if(page==SCRIPT_INFO){
  if(y<198||y>=233)return;
  if(x<74){page=libraryFolderUart?UART_FILES:BADUSB_FILES;draw();}
  else if(x<146){libraryGuide=!libraryGuide;libraryLoad();draw();}
  else if(x<224){
#if V61_ENABLE_NATIVE_HID
   if(String(V62ScriptAssets::files[librarySelected].runtime)=="badusb"&&!V61USBHID::enabled){V61USBHID::enabled=V61USBHID::begin();libraryText="USB HID started. Connect native USB to your test PC, then tap RUN. STOP releases keys. No script has run yet.";libraryIndex();libraryTextOffset=0;draw();return;}
#endif
   auto f=V62ScriptAssets::files[librarySelected];String body=V62ScriptApps::content(f);f.content=body.c_str();bool ok=V62Scripts::start(f);libraryText=(ok?String("Started. STOP remains available.\n\n"):String("Not started:\n\n"))+V62Scripts::message+"\n\nUse Hardware -> USB HID start for BadUSB, or UART start for UART scripts. Swipe/drag the right scrollbar for instructions.";libraryIndex();libraryTextOffset=0;draw();
  }else{V62Scripts::stop();libraryText=V62Scripts::message;libraryIndex();libraryTextOffset=0;draw();}
 }else{
  auto files=libraryFiles();
  if(y>=198&&y<233){if(x<112)goBack();else if(x>=241){V62Scripts::stop();draw();}else if(x>=181&&libraryOffset+5<files.size()){libraryOffset+=5;draw();}else if(x>=121&&x<175){libraryOffset=libraryOffset>=5?libraryOffset-5:0;draw();}}
  else if(x<302&&y>=52&&y<192){size_t index=libraryOffset+(y-52)/28;if(index<files.size()){librarySelected=files[index];libraryGuide=true;libraryLoad();page=SCRIPT_INFO;draw();}}
 }
}
