#pragma once
#include <string>
#include <functional>
#include <map>
#include <vector>
class String:public std::string {
public:
 using std::string::string;
 String()=default;
 String(const std::string &s):std::string(s){}
 String substring(size_t start,size_t end)const{return substr(start,end-start);}
 String substring(size_t start)const{return substr(start);}
};
enum HTTPMethod{HTTP_GET,HTTP_POST};
class WebServer {
public:
 using THandlerFunction=std::function<void()>;
 explicit WebServer(int){}
 int networkCode=0;
 String networkBody;
 std::map<String,String> networkArgs,networkHeaders;
 std::vector<THandlerFunction> handlers;
 void on(const char*,HTTPMethod,THandlerFunction fn){handlers.push_back(fn);}
 String arg(const String &name){return networkArgs[name];}
 String header(const String &name){return networkHeaders[name];}
 void send(int code,const char*,const String &body){networkCode=code;networkBody=body;}
 void send_P(int code,const char*,const char *body,size_t n){networkCode=code;networkBody=String(body,n);}
 void sendHeader(const String&,const String&,bool=false){}
 template<class F> size_t streamFile(F &file,const String&){return file.size();}
};
