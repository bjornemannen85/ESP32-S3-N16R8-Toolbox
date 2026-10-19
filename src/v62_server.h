#pragma once
#include <WebServer.h>
#include <functional>
#include <vector>

// Both interfaces invoke exactly the same validated handlers on the Arduino
// loop task. No HTTP loopback, duplicated driver state, or background GPIO task.
class V62Server : public WebServer {
public:
 struct Entry { String path; HTTPMethod method; std::function<void()> run; };
 struct Field { String name, value; };
 std::vector<Entry> programs;
 String result;
 int resultCode=0;
 static constexpr size_t RESULT_LIMIT=49152;
 explicit V62Server(int port):WebServer(port){}
 void on(const char *path, HTTPMethod method, THandlerFunction fn) {
  bool found=false;
  for(const auto &p:programs)if(p.path==path&&p.method==method){found=true;break;}
  if(!found)programs.push_back({String(path),method,fn});
  WebServer::on(path,method,fn);
 }
 bool invoke(size_t index,const std::vector<Field> &fields,const String &token) {
  if(local||index>=programs.size())return false;
  local=true;arguments=fields;localToken=token;result="";resultCode=0;
  programs[index].run();
  local=false;arguments.clear();localToken="";
  return true;
 }
 String arg(const String &name) {
  if(!local)return WebServer::arg(name);
  for(const auto &f:arguments)if(f.name==name)return f.value;
  return String();
 }
 String header(const String &name) {
  if(!local)return WebServer::header(name);
  return name=="X-Cyberdeck-Token"?localToken:String();
 }
 void send(int code,const char *type,const String &body) {
  if(!local){WebServer::send(code,type,body);return;}
  resultCode=code;result=body.substring(0,RESULT_LIMIT);
  if(body.length()>RESULT_LIMIT)result+="\n[display truncated]";
 }
 void sendHeader(const String &name,const String &value,bool first=false) {
  if(!local)WebServer::sendHeader(name,value,first);
 }
 template<class FileType> size_t streamFile(FileType &file,const String &type) {
  if(!local)return WebServer::streamFile(file,type);
  result="";resultCode=200;
  while(file.available()&&result.length()<RESULT_LIMIT)result+=(char)file.read();
  return result.length();
 }
private:
 bool local=false;
 String localToken;
 std::vector<Field> arguments;
};
