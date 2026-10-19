#pragma once
#include "v62_access.h"
static String v62Filter="";
static std::vector<size_t> v62Visible;
static std::vector<V62Server::Field> v62Arguments;
static size_t v62Offset=0,v62Selected=0,v62ResultOffset=0;
static bool v62WasReceiver=false;
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
    (!v62Filter.length()||p.path.indexOf(v62Filter)>=0))v62Visible.push_back(i);
 }
}
void v62Open(const String &filter){
 v62Filter=filter;v62Offset=0;v62Rebuild();
 if(historyDepth<12)history[historyDepth++]=page;
 page=PROGRAMS;draw();
}
static void v62Choose(size_t index){
 v62Selected=index;v62Arguments.clear();
 String fields=v62Fields(v6Server.programs[index]);
 int pos=0;
 while(pos<(int)fields.length()){
  int end=fields.indexOf(',',pos);if(end<0)end=fields.length();
  v62Arguments.push_back({fields.substring(pos,end),""});pos=end+1;
 }
 page=PROGRAM_ACTION;draw();
}
void v62Draw(){
 if(page==PROGRAMS){
  v62Rebuild();
  label(String(v62Visible.size())+" programs "+v62Filter,8,38,1,MUTED);
  for(size_t row=0;row<5&&v62Offset+row<v62Visible.size();row++){
   const auto &p=v6Server.programs[v62Visible[v62Offset+row]];
   int y=52+row*28;lcd.drawRoundRect(6,y,308,26,4,ACCENT);
   label((p.method==HTTP_POST?"* ":"  ")+clipped(v62ProgramName(p.path),46),10,y+8,1);
  }
  backButton();button(121,198,54,35,"<");button(181,198,54,35,">");button(241,198,72,35,"ALL");
 }else if(page==PROGRAM_ACTION){
  const auto &p=v6Server.programs[v62Selected];
  label(clipped(v62ProgramName(p.path),50),8,39,1,ACCENT);
  if(v62Arguments.empty())label("No parameters. Tap RUN.",8,65,1);
  for(size_t i=0;i<v62Arguments.size()&&i<4;i++){
   const auto &f=v62Arguments[i];int y=56+i*29;
   lcd.drawRoundRect(6,y,308,27,4,ACCENT);
   bool secret=f.name=="password"||f.name=="token";
   label(clipped(f.name+": "+(secret?String(f.value.length(),'*'):f.value),48),10,y+8,1);
  }
  label(p.method==HTTP_POST?"Manual action: RUN applies changes":"Tap any field to edit",8,182,1,ORANGE);
  backButton();button(181,198,132,35,"RUN");
 }else{
  label("Status "+String(v6Server.resultCode),8,38,1,ACCENT);
  size_t pos=v62ResultOffset;
  for(int row=0;row<17&&pos<v6Server.result.length();row++){
   String line="";
   while(pos<v6Server.result.length()&&line.length()<50){char c=v6Server.result[pos++];if(c=='\n')break;if(c!='\r')line+=c;}
   label(line,8,51+row*8,1);
  }
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
   if(x>=241){v62Filter="";v62Offset=0;}
   else if(x>=181&&v62Offset+5<v62Visible.size())v62Offset+=5;
   else if(x>=121&&x<175)v62Offset=v62Offset>=5?v62Offset-5:0;
   draw();
  }else if(page==PROGRAM_ACTION&&x>=181){
   v6Server.invoke(v62Selected,v62Arguments,V61WiFi::adminToken);
   v62SyncBle();v62ResultOffset=0;page=PROGRAM_RESULT;draw();
  }else if(page==PROGRAM_RESULT){
   if(x>=241){page=PROGRAMS;draw();}
   else{
    // Page by actual rendered line breaks, including wrapped JSON/text.
    if(x>=181){size_t pos=v62ResultOffset;for(int row=0;row<17&&pos<v6Server.result.length();row++){int n=0;while(pos<v6Server.result.length()&&n++<50)if(v6Server.result[pos++]=='\n')break;}if(pos<v6Server.result.length())v62ResultOffset=pos;}
    else if(x>=121)v62ResultOffset=0;
    draw();
   }
  }
 }else if(page==PROGRAMS&&y>=52&&y<192){
  size_t row=(y-52)/28;if(v62Offset+row<v62Visible.size())v62Choose(v62Visible[v62Offset+row]);
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
