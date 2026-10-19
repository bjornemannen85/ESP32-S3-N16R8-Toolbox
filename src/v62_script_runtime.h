#pragma once
#include "v62_macro_core.h"
#include "v62_script_assets.h"
#include "v61_bundle81100.h"
#include "v61_usb_hid.h"
namespace V62Scripts {
static V62Macro::Program program;
static bool running=false,uartMode=false;
static size_t step=0,character=0;
static unsigned long due=0,started=0;
static unsigned defaultDelay=0,stringDelay=10;
static String message="Idle",filename="";
static const V62ScriptAssets::File *find(const String &folder,const String &name){for(const auto &f:V62ScriptAssets::files)if(folder==f.folder&&name==f.name)return &f;return nullptr;}
static String quoted(const String &s){return String("\"")+V61WiFi::escapeJson(s)+"\"";}
static void stop(){running=false;program.steps.clear();step=character=0;V61USBHID::stop();message="Stopped; HID keys released";}
static String state(){return String("{\"running\":")+(running?"true":"false")+",\"mode\":"+quoted(uartMode?"uart":"badusb")+",\"name\":"+quoted(filename)+",\"step\":"+String(step)+",\"total\":"+String(program.steps.size())+",\"message\":"+quoted(message)+"}";}
#if V61_ENABLE_NATIVE_HID
static uint8_t keyCode(const std::string &name){
 if(name.size()==1)return uint8_t(name[0]);
 const char *names[]={"CTRL","CONTROL","ALT","SHIFT","GUI","WINDOWS","COMMAND","ENTER","RETURN","TAB","ESC","ESCAPE","SPACE","UP","DOWN","LEFT","RIGHT","HOME","END","PAGEUP","PAGEDOWN","DELETE","BACKSPACE","INSERT","CAPSLOCK"};
 const uint8_t codes[]={KEY_LEFT_CTRL,KEY_LEFT_CTRL,KEY_LEFT_ALT,KEY_LEFT_SHIFT,KEY_LEFT_GUI,KEY_LEFT_GUI,KEY_LEFT_GUI,KEY_RETURN,KEY_RETURN,KEY_TAB,KEY_ESC,KEY_ESC,' ',KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW,KEY_RIGHT_ARROW,KEY_HOME,KEY_END,KEY_PAGE_UP,KEY_PAGE_DOWN,KEY_DELETE,KEY_BACKSPACE,KEY_INSERT,KEY_CAPS_LOCK};
 for(size_t i=0;i<sizeof(codes);i++)if(name==names[i])return codes[i];
 if(name.size()>1&&name[0]=='F')return KEY_F1+std::atoi(name.c_str()+1)-1;return 0;
}
static uint8_t mouseKey(const std::string &key){return key=="LEFTCLICK"?MOUSE_LEFT:key=="RIGHTCLICK"?MOUSE_RIGHT:key=="MIDDLECLICK"?MOUSE_MIDDLE:0;}
static void keys(const std::string &value,bool release=false){std::istringstream tokens(value);std::string name;while(tokens>>name){uint8_t mouse=mouseKey(name);if(mouse){if(release)V61USBHID::mouse.release(mouse);else V61USBHID::mouse.press(mouse);}else if(release)V61USBHID::keyboard.release(keyCode(name));else V61USBHID::keyboard.press(keyCode(name));}}
static void altChar(unsigned value){String digits(value);V61USBHID::keyboard.press(KEY_LEFT_ALT);for(char digit:digits){uint8_t key=digit=='0'?KEY_KP_0:KEY_KP_1+(digit-'1');V61USBHID::keyboard.press(key);V61USBHID::keyboard.release(key);}V61USBHID::keyboard.release(KEY_LEFT_ALT);}
#endif
static bool start(const V62ScriptAssets::File &file){
 if(running){message="Stop the current script first";return false;}
 bool uart=String(file.runtime)=="uart";if(!uart&&String(file.runtime)!="badusb"){message="Reference sketch: flash on a separate Arduino board; not an ESP32 macro";return false;}
 program=V62Macro::Program{};
 auto parsed=V62Macro::parse(file.content,uart);if(!parsed.error.empty()){message=String("Line ")+parsed.errorLine+": "+parsed.error.c_str();return false;}
 if(uart&&!V61Bundle81100::active){message="Start UART with RX/TX/baud first";return false;}
 if(!uart&&!V61USBHID::enabled){message="Start native USB HID manually first";return false;}
 program=std::move(parsed);running=true;uartMode=uart;filename=file.name;step=character=0;defaultDelay=0;stringDelay=10;due=millis()+1000;started=millis();message="Starting in one second";return true;
}
static void tick(){
 if(!running||int32_t(millis()-due)<0)return;
 if(millis()-started>600000){stop();message="Ten-minute runtime limit reached";return;}
 if(uartMode&&!V61Bundle81100::active){stop();message="UART stopped";return;}
 if(!uartMode&&!V61USBHID::enabled){stop();message="HID stopped";return;}
 if(step>=program.steps.size()){running=false;message="Completed";
#if V61_ENABLE_NATIVE_HID
  V61USBHID::keyboard.releaseAll();V61USBHID::mouse.release(MOUSE_ALL);
#endif
  return;
 }
 const auto &s=program.steps[step];message=String("Line ")+s.line+": "+s.op.c_str();unsigned wait=defaultDelay;
 if(s.op=="DELAY"||s.op=="WAIT_RX")wait=std::atoi(s.arg.c_str());
 else if(uartMode){
  std::string data;if(s.op=="TX")data=V62Macro::unescape(s.arg);else if(s.op=="TXHEX"){std::vector<uint8_t> raw;V62ModuleCore::hex(s.arg,raw,128);data.assign(raw.begin(),raw.end());}
  size_t remaining=data.size()-character;int free=V61Bundle81100::port.availableForWrite();
  if(remaining&&free<=0){due=millis()+1;return;}
  size_t count=std::min(remaining,std::min(size_t(free<0?0:free),size_t(32)));
  size_t n=count?V61Bundle81100::port.write((const uint8_t*)data.data()+character,count):0;V61Bundle81100::transmitted+=n;character+=n;
  if(n!=count){stop();message="UART write incomplete";return;}
  if(character<data.size()){due=millis()+1;return;}character=0;
 }else{
#if V61_ENABLE_NATIVE_HID
  if(!USB){stop();message="Native USB host disconnected";return;}
  if(s.op=="ID"){wait=0;}
  else if(s.op=="DEFAULT_DELAY"||s.op=="DEFAULTDELAY"){defaultDelay=std::atoi(s.arg.c_str());wait=0;}
  else if(s.op=="DEFAULT_STRING_DELAY"||s.op=="STRING_DELAY"){stringDelay=std::atoi(s.arg.c_str());wait=0;}
  else if(s.op=="STRING"||s.op=="STRINGLN"||s.op=="ALTSTRING"){
   if(character<s.arg.size()){unsigned char ch=s.arg[character++];if(s.op=="ALTSTRING")altChar(ch);else V61USBHID::keyboard.write(ch);due=millis()+stringDelay;return;}
   if(s.op=="STRINGLN")V61USBHID::keyboard.write(KEY_RETURN);character=0;
  }else if(s.op=="ALTCHAR")altChar(std::atoi(s.arg.c_str()));
  else if(s.op=="MOUSEMOVE"){std::istringstream nums(s.arg);int x=0,y=0;nums>>x>>y;V61USBHID::mouse.move(x,y);}
  else if(s.op=="MOUSESCROLL")V61USBHID::mouse.move(0,0,std::atoi(s.arg.c_str()));
  else if(s.op=="HOLD")keys(s.arg);
  else if(s.op=="RELEASE")keys(s.arg,true);
  else if(s.op=="KEYS"){keys(s.arg);V61USBHID::keyboard.releaseAll();V61USBHID::mouse.release(MOUSE_ALL);}
#endif
 }
 step++;due=millis()+wait;
}
}
