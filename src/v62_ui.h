#pragma once
#include "v62_access.h"
#include <algorithm>
#include "v62_text.h"
static String v62Filter="";
static int v62Category=V62Menu::All;
struct V62Shortcut { const char *title; Page target; };
static std::vector<V62Shortcut> v62Shortcuts;
String v62MenuTitle(){return v62Category<0?String("ALL TOOLS"):String(V62Menu::labels[v62Category]);}
static std::vector<size_t> v62Visible;
static size_t v62Count(){return v62Shortcuts.size()+v62Visible.size();}
static std::vector<V62Server::Field> v62Arguments;
static size_t v62Offset=0,v62Selected=0,v62ResultOffset=0;
struct V62Navigation {bool valid=false;int category=V62Menu::All;String filter;size_t offset=0;std::vector<V62Shortcut> shortcuts;};
static V62Navigation v62Navigation[12];
void v62SaveNavigation(int index){if(index<0||index>=12)return;auto &n=v62Navigation[index];n.valid=page==PROGRAMS;if(n.valid){n.category=v62Category;n.filter=v62Filter;n.offset=v62Offset;n.shortcuts=v62Shortcuts;}}
void v62RestoreNavigation(int index){if(index<0||index>=12||!v62Navigation[index].valid)return;const auto &n=v62Navigation[index];v62Category=n.category;v62Filter=n.filter;v62Offset=n.offset;v62Shortcuts=n.shortcuts;}

static bool v62WasReceiver=false;
static size_t v62ResultLineCount=0;
static void v62IndexResult(){v62ResultLineCount=V62Text::lines(v6Server.result.c_str(),v6Server.result.length());}
static size_t v62ResultLine(){return V62Text::line(v6Server.result.c_str(),v6Server.result.length(),v62ResultOffset);}
static size_t v62ResultByte(size_t line){return V62Text::offset(v6Server.result.c_str(),v6Server.result.length(),line);}

bool v62ScrollTouch(bool down,uint16_t x,uint16_t y){
 if(!down){bool was=scrollHeld;scrollHeld=false;return was;}
 V62Scroll::Bar bar;
 if(page==PROGRAMS){bar.total=v62Count();bar.visible=5;bar.offset=v62Offset;}
 else if(page==PROGRAM_RESULT){bar.total=v62ResultLineCount;bar.visible=17;bar.offset=v62ResultLine();}
 else if(page==WIFI||page==BLE){bar.total=page==WIFI?wifiStored:bleStored;bar.visible=5;bar.offset=page==WIFI?wifiOffset:bleOffset;}
 else{scrollHeld=false;return false;}
 if(!scrollHeld&&(!bar.hit(x,y)||lastPressed))return false;
 scrollHeld=true;size_t next=bar.seek(y);
 if(next!=bar.offset){
  if(page==PROGRAMS)v62Offset=next;
  else if(page==PROGRAM_RESULT)v62ResultOffset=v62ResultByte(next);
  else if(page==WIFI)wifiOffset=next;else bleOffset=next;
  draw();
 }
 return true;
}
static void v62SyncBle(){
 // BLE stop deinitializes the singleton; scanning must initialize it again.
 if(v62WasReceiver&&!V61BleTransfer::running)bleReady=false;
 if(V61BleTransfer::running)bleReady=true;
 v62WasReceiver=V61BleTransfer::running;
}
static void v62Rebuild(){
 v62Visible.clear();
 for(size_t i=0;i<v6Server.programs.size();i++){
  const auto &p=v6Server.programs[i];
  if(!v62IsPage(p.path)&&!p.path.startsWith("/access")&&
    (v62Category<0||V62Menu::category(p.path.c_str())==v62Category)&&
    (!v62Filter.length()||p.path.indexOf(v62Filter)>=0))v62Visible.push_back(i);
 }
 std::sort(v62Visible.begin(),v62Visible.end(),[](size_t a,size_t b){
  return v62ProgramName(v6Server.programs[a].path).compareTo(v62ProgramName(v6Server.programs[b].path))<0;
 });
}
void v62Open(const String &filter){
 if(historyDepth<12){v62SaveNavigation(historyDepth);history[historyDepth++]=page;}
 v62Filter=filter;v62Category=V62Menu::All;v62Shortcuts.clear();v62Offset=0;v62Rebuild();
 page=PROGRAMS;draw();
}
void v62OpenCategory(int category){
 if(historyDepth<12){v62SaveNavigation(historyDepth);history[historyDepth++]=page;}
 v62Category=category;v62Filter="";v62Offset=0;v62Shortcuts.clear();
 if(category==V62Menu::Network)v62Shortcuts.push_back({"WiFi scanner",WIFI});
 if(category==V62Menu::Bluetooth)v62Shortcuts.push_back({"BLE scanner",BLE});
 if(category==V62Menu::System){
  v62Shortcuts.push_back({"System monitor",SYSTEM});v62Shortcuts.push_back({"Event log",LOGS});
  v62Shortcuts.push_back({"Touch test",TOUCH});
 }
 if(category==V62Menu::Files){v62Shortcuts.push_back({"BadUSB script library",BADUSB_FILES});v62Shortcuts.push_back({"UART script library",UART_FILES});}
 if(category==V62Menu::Hardware)v62Shortcuts.push_back({"UART scripts",UART_FILES});
 if(category==V62Menu::Settings){
  v62Shortcuts.push_back({"Connection / admin settings",SETTINGS});
  v62Shortcuts.push_back({"WiFi setup",WIFI_SETUP});
 }
 if(category==V62Menu::Modules){
  v62Shortcuts.push_back({"NFC / PN532 status",NFC});v62Shortcuts.push_back({"CC1101 status",SUBGHZ});
  v62Shortcuts.push_back({"IR status",IR});v62Shortcuts.push_back({"Module inventory",MODULES});
 }
 v62Rebuild();
 page=PROGRAMS;draw();
}
static void v62Choose(size_t index){
 v62Selected=index;v62Arguments.clear();
 String fields=v62Fields(v6Server.programs[index]);
 int pos=0;
 while(pos<(int)fields.length()){
  int end=fields.indexOf(',',pos);if(end<0)end=fields.length();
  String name=fields.substring(pos,end),value="";const String &path=v6Server.programs[index].path;
  if(name=="id")value=v62ModuleContext;
  if(name=="frequency")value="433920000";
  if(path=="/api/v61/uart/start"){if(name=="rx")value="4";if(name=="tx")value="5";if(name=="baud")value="9600";}
  if(path=="/api/v61/adc/read"&&name=="pin")value="1";
  if(name=="host")value="example.com";
  if(path=="/wifi/http"&&name=="url")value="http://example.com";
  if(name=="folder")value="badusb";
  v62Arguments.push_back({name,value});pos=end+1;
 }
 page=PROGRAM_ACTION;draw();
}
void v62Draw(){
 lcd.fillRect(6,4,24,24,PANEL);
 V62Icons::draw(lcd,page==PROGRAMS?V62Icons::categoriesIcon(v62Category):V62Icons::route(v6Server.programs[v62Selected].path.c_str()),6,4);
 if(page==PROGRAMS){
  v62Rebuild();
  label(String(v62Count())+" tools "+v62Filter,8,38,1,MUTED);
  for(size_t row=0;row<5&&v62Offset+row<v62Count();row++){
   size_t index=v62Offset+row;String title;
   if(index<v62Shortcuts.size())title=String("> ")+v62Shortcuts[index].title;
   else{const auto &p=v6Server.programs[v62Visible[index-v62Shortcuts.size()]];
    title=(p.method==HTTP_POST?"* ":"  ")+v62ProgramName(p.path);
   }
   int y=52+row*28;lcd.drawRoundRect(6,y,292,26,4,ACCENT);
   const auto &icon=index<v62Shortcuts.size()?V62Icons::pagesIcon(v62Shortcuts[index].target):V62Icons::route(v6Server.programs[v62Visible[index-v62Shortcuts.size()]].path.c_str());
   V62Icons::draw(lcd,icon,8,y+1);label(clipped(title,42),38,y+8,1);
  }
  V62Scroll::Bar{v62Count(),5,v62Offset}.draw(lcd,PANEL,ACCENT);
  backButton();button(121,198,54,35,"<");button(181,198,54,35,">");button(241,198,72,35,"CATS");
 }else if(page==PROGRAM_ACTION){
  const auto &p=v6Server.programs[v62Selected];
  label(clipped(v62ProgramName(p.path),50),8,39,1,ACCENT);
  if(v62Arguments.empty())label("No parameters. Tap RUN.",8,65,1);
  for(size_t i=0;i<v62Arguments.size()&&i<4;i++){
   const auto &f=v62Arguments[i];int y=56+i*29;
   lcd.drawRoundRect(6,y,308,27,4,ACCENT);
   bool secret=f.name=="password"||f.name=="token";
   label(clipped(f.name+": "+(secret?v62Mask(f.value.length()):f.value),48),10,y+8,1);
  }
  label(p.method==HTTP_POST?"Manual action: RUN applies changes":"Tap any field to edit",8,182,1,ORANGE);
  backButton();button(181,198,132,35,"RUN");
 }else{
  label("Status "+String(v6Server.resultCode),8,38,1,ACCENT);
  size_t pos=v62ResultOffset;
  for(int row=0;row<17&&pos<v6Server.result.length();row++){
   String line="";
   while(pos<v6Server.result.length()&&line.length()<48){char c=v6Server.result[pos++];if(c=='\n')break;if(c!='\r')line+=c;}
   label(line,8,51+row*8,1);
  }
  V62Scroll::Bar{v62ResultLineCount,17,v62ResultLine()}.draw(lcd,PANEL,ACCENT);
  backButton();button(121,198,54,35,"<");button(181,198,54,35,">");button(241,198,72,35,"APPS");
 }
}
void v62Touch(uint16_t x,uint16_t y){
 if(y>=198&&y<233){
  if(x>=8&&x<112){
   if(page==PROGRAMS)goBack();
   else if(page==PROGRAM_RESULT){page=PROGRAM_ACTION;draw();}
   else{page=PROGRAMS;draw();}
  }else if(page==PROGRAMS){
   if(x>=241){page=HOME;historyDepth=0;draw();return;}
   else if(x>=181&&v62Offset+5<v62Count())v62Offset+=5;
   else if(x>=121&&x<175)v62Offset=v62Offset>=5?v62Offset-5:0;
   draw();
  }else if(page==PROGRAM_ACTION&&x>=181){
   v6Server.invoke(v62Selected,v62Arguments);
   v62SyncBle();v62ResultOffset=0;v62IndexResult();page=PROGRAM_RESULT;draw();
  }else if(page==PROGRAM_RESULT){
   if(x>=241){page=PROGRAMS;draw();}
   else{
    size_t line=v62ResultLine();
    if(x>=181&&line+17<v62ResultLineCount)v62ResultOffset=v62ResultByte(line+17);
    else if(x>=121&&x<175)v62ResultOffset=v62ResultByte(line>=17?line-17:0);
    draw();
   }
  }
 }else if(page==PROGRAMS&&x<302&&y>=52&&y<192){
  size_t index=v62Offset+(y-52)/28;
  if(index<v62Shortcuts.size())enter(v62Shortcuts[index].target,PROGRAMS);
  else if(index<v62Count())v62Choose(v62Visible[index-v62Shortcuts.size()]);
 }else if(page==PROGRAM_ACTION&&y>=56&&y<172){
  size_t i=(y-56)/29;if(i<v62Arguments.size()){
   wifiReturnPage=PROGRAM_ACTION;wifiField=4;v62KeyTitle=v62Arguments[i].name;
   wifiOpenKeyboard(v62Arguments[i].value);
  }
 }
}
static void v62Register(){
 v62AccessRoutes();
 v6Server.on("/access/ble",HTTP_POST,[](){
  if(!V61WiFi::authorized(v6Server))return V61WiFi::forbidden(v6Server);
  if(V61BleTransfer::running){v6Server.send(409,"text/plain","Stop BLE receiver before scanning");return;}
  Page saved=page;scanBle();page=saved;draw();
  String j="[";for(int i=0;i<bleStored;i++){if(i)j+=",";j+="{\"name\":\""+V61WiFi::escapeJson(bleLines[i])+"\",\"rssi\":"+String(bleRssi[i])+"}";}v6Server.send(200,"application/json",j+"]");
 });
}
