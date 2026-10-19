#pragma once
#include "v62_module_core.h"
#include <sstream>
namespace V62Macro {
struct Step{std::string op,arg;unsigned line=0;};
struct Program{std::vector<Step> steps;std::string error;unsigned errorLine=0;};
inline std::string trim(std::string s){size_t a=s.find_first_not_of(" \t\r");if(a==std::string::npos)return "";size_t b=s.find_last_not_of(" \t\r");return s.substr(a,b-a+1);}
inline bool key(const std::string &s){
 if(s.size()==1&&s[0]>=32&&s[0]<=126)return true;
 const char *keys[]={"CTRL","CONTROL","ALT","SHIFT","GUI","WINDOWS","COMMAND","ENTER","RETURN","TAB","ESC","ESCAPE","SPACE","UP","DOWN","LEFT","RIGHT","HOME","END","PAGEUP","PAGEDOWN","DELETE","BACKSPACE","INSERT","CAPSLOCK","LEFTCLICK","RIGHTCLICK","MIDDLECLICK"};
 for(auto k:keys)if(s==k)return true;
 if(s.size()>=2&&s[0]=='F'){long n;return V62ModuleCore::integer(s.substr(1),n,1,12);}return false;
}
inline bool combo(const std::string &s){std::istringstream in(s);std::string k;int n=0;while(in>>k){if(!key(k)||++n>6)return false;}return n>0;}
inline Program parse(const std::string &body,bool uart){
 Program p;if(body.size()>8192){p.error="8 KiB script limit";return p;}
 std::istringstream lines(body);std::string line;unsigned number=0;Step previous;bool havePrevious=false;size_t expandedBytes=0;
 while(std::getline(lines,line)){
  number++;if(!line.empty()&&line.back()=='\r')line.pop_back();line=trim(line);
  if(line.empty()||line[0]=='#'||line=="REM"||line.rfind("REM ",0)==0)continue;
  size_t sp=line.find(' ');std::string op=line.substr(0,sp),arg=sp==std::string::npos?"":line.substr(sp+1);bool ok=true;long n=0;
  if(op=="REPEAT"){
   ok=havePrevious&&V62ModuleCore::integer(arg,n,1,1000)&&p.steps.size()+size_t(n)<=256&&expandedBytes+size_t(n)*previous.arg.size()<=16384;
   if(ok){for(long i=0;i<n;i++){Step repeated=previous;repeated.line=number;p.steps.push_back(repeated);expandedBytes+=repeated.arg.size();}continue;}
  }else if(op=="DELAY"||(!uart&&(op=="DEFAULT_DELAY"||op=="DEFAULT_STRING_DELAY"||op=="DEFAULTDELAY"||op=="STRING_DELAY"))){ok=V62ModuleCore::integer(arg,n,0,60000);}
  else if(uart&&op=="TXHEX"){std::vector<uint8_t> bytes;ok=V62ModuleCore::hex(arg,bytes,128);}
  else if(uart&&op=="TX"){ok=arg.size()<=256;}
  else if(uart&&op=="WAIT_RX"){ok=V62ModuleCore::integer(arg,n,1,60000);}
  else if(!uart&&(op=="STRING"||op=="STRINGLN"||op=="ALTSTRING")){ok=arg.size()<=1024;}
  else if(!uart&&op=="ALTCHAR"){ok=V62ModuleCore::integer(arg,n,1,255);}
  else if(!uart&&op=="ID"){ok=!arg.empty();}
  else if(!uart&&(op=="MOUSEMOVE"||op=="MOUSESCROLL")){std::istringstream nums(arg);std::string a,b,extra;nums>>a;if(op=="MOUSEMOVE")nums>>b;nums>>extra;long v;ok=extra.empty()&&V62ModuleCore::integer(a,v,-127,127)&&(op=="MOUSESCROLL"||V62ModuleCore::integer(b,v,-127,127));}
  else if(!uart&&(op=="HOLD"||op=="RELEASE")){ok=combo(arg);}
  else if(!uart&&combo(line)){arg=line;op="KEYS";}
  else ok=false;
  if(!ok){p.error="Unsupported command or invalid argument: "+line;p.errorLine=number;return p;}
  if(p.steps.size()>=256||expandedBytes+arg.size()>16384){p.error="256 expanded step limit";p.errorLine=number;return p;}
  Step step{op,arg,number};p.steps.push_back(step);expandedBytes+=arg.size();
  if(op!="ID"&&op!="DEFAULT_DELAY"&&op!="DEFAULTDELAY"&&op!="DEFAULT_STRING_DELAY"&&op!="STRING_DELAY"){previous=step;havePrevious=true;}
 }
 if(p.steps.empty())p.error="Script is empty";
 return p;
}
inline std::string unescape(const std::string &s){std::string out;for(size_t i=0;i<s.size();i++){if(s[i]=='\\'&&i+1<s.size()){char c=s[++i];out+=c=='r'?'\r':c=='n'?'\n':c=='t'?'\t':c;}else out+=s[i];}return out;}
}
