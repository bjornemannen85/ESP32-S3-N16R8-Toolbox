#include "v62_server.h"
#include <cassert>
#include <iostream>
struct File {
 String body;size_t pos=0;
 bool available()const{return pos<body.length();}
 char read(){return body[pos++];}
 size_t size()const{return body.size();}
};
int main(){
 V62Server s(80);int writes=0;
 auto handler=[&](){
  if(s.arg("rx")!="4"){s.send(400,"text/plain","invalid pin");return;}
  ++writes;s.send(200,"text/plain",s.arg("data"));
 };
 s.on("/uart/start",HTTP_POST,handler);
 assert(s.programs.size()==1);
 // A physical touch call must pass the existing authentication and validation.
 assert(s.invoke(0,{{"rx","4"},{"data","hello"}}));
 assert(s.resultCode==200&&writes==1);
 assert(s.invoke(0,{{"rx","19"}}));
 assert(s.resultCode==400&&writes==1);
 assert(s.invoke(0,{{"rx","4"},{"data","hello"}}));
 assert(s.resultCode==200&&s.result=="hello"&&writes==2);
 // Local arguments and credentials must never leak into a later web request.
 assert(s.arg("rx").empty());
 s.handlers[0]();assert(s.networkCode==400&&writes==2);
 s.networkArgs["rx"]="4";s.networkArgs["data"]="web";
 s.handlers[0]();assert(s.networkBody=="web"&&writes==3);
 // Repeated paths in the legacy bundle use the same first-match semantics.
 s.on("/uart/start",HTTP_POST,[&](){++writes;});
 assert(s.programs.size()==1);
 assert(!s.invoke(100,{}));assert(writes==3);
 // Local preview memory is bounded, even for downloads/backup output.
 s.on("/large",HTTP_GET,[&](){s.send(200,"text/plain",String(60000,'x'));});
 assert(s.invoke(1,{}));assert(s.result.size()<50000);
 File file{String("file data"),0};
 s.on("/file",HTTP_GET,[&](){s.streamFile(file,"text/plain");});
 assert(s.invoke(2,{}));assert(s.result=="file data"&&s.resultCode==200);
 // Nested dispatch must fail without corrupting the active request context.
 s.on("/nested",HTTP_GET,[&](){assert(!s.invoke(0,{}));s.send(201,"text/plain","outer");});
 assert(s.invoke(3,{}));assert(s.resultCode==201&&s.result=="outer");
 s.send(200,"text/html","<!doctype html><html><head><style>body{color:white}</style></head><body>hello</body></html>");
 assert(s.networkBody.find("/wolfdeck_theme.css")!=String::npos);
 assert(s.networkBody.find("<!doctype html>")==0);
 assert(s.networkBody.find("/wolfdeck_theme.css")<s.networkBody.find("</head>"));
 s.send(200,"application/json","{\"ok\":true}");assert(s.networkBody=="{\"ok\":true}");
 s.on("/flash",HTTP_GET,[&](){s.send_P(200,"text/plain","guide from flash",16);});
 assert(s.invoke(4,{}));assert(s.resultCode==200&&s.result=="guide from flash");
 std::cout<<"PASS: shared handlers, authentication, validation, context isolation, duplicate routes, bounds, streaming, flash content capture, reentrancy\n";
}
