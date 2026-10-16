#pragma once
#include <LittleFS.h>
#include <WebServer.h>
#include "v61_wifi.h"

// Script storage only. Never executes uploaded content.
namespace V61Scripts {
static bool ready=false;
static const size_t MAX_SCRIPT=8192;
static bool validName(const String &name){
 if(name.length()<3||name.length()>48||name[0]=='.')return false;
 for(size_t i=0;i<name.length();++i){char c=name[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;}
 const char *exts[]={".duck",".txt",".ds",".json",".yaml",".ini",".csv",".lua",".js",".py",".sh",".bat",".ps1",".ino",".cpp",".hex",".sub",".ir",".nfc",".rfid"};
 for(const char *ext:exts)if(name.endsWith(ext))return true;
 return false;
}
static String path(const String &name){return String("/scripts/")+name;}
static void begin(){ready=LittleFS.begin(false);if(ready&&!LittleFS.exists("/scripts"))ready=LittleFS.mkdir("/scripts");}
// V61_STAGE6_TRANSFER: authenticated Wi-Fi export and non-executing macro preview.
static String preview(const String &body){
 String out="{\"execution\":false,\"commands\":[";int pos=0,count=0;
 while(pos<(int)body.length()&&count<100){int end=body.indexOf('\n',pos);if(end<0)end=body.length();
  String line=body.substring(pos,end);line.trim();pos=end+1;
  if(!line.length()||line.startsWith("REM ")||line.startsWith("#"))continue;
  int sp=line.indexOf(' ');String op=sp<0?line:line.substring(0,sp);op.toUpperCase();
  bool supported=op=="STRING"||op=="DELAY"||op=="ENTER"||op=="TAB"||op=="ESC";
  if(count++)out+=",";out+="{\"line\":\""+V61WiFi::escapeJson(line)+"\",\"recognized\":"+(supported?"true":"false")+"}";
 }
 return out+"]}";
}
static void routes(WebServer &s){
 s.on("/scripts/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",String("{\"ready\":")+(ready?"true":"false")+",\"storage\":\"LittleFS\",\"execution\":false}");});
 s.on("/scripts/list",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);if(!ready){s.send(503,"text/plain","LittleFS unavailable");return;}File dir=LittleFS.open("/scripts");if(!dir||!dir.isDirectory()){s.send(500,"text/plain","Directory unavailable");return;}String out="[";bool first=true;File f=dir.openNextFile();while(f){String n=String(f.name());int slash=n.lastIndexOf('/');if(slash>=0)n=n.substring(slash+1);if(!f.isDirectory()&&validName(n)){if(!first)out+=",";first=false;out+="{\"name\":\""+V61WiFi::escapeJson(n)+"\",\"bytes\":"+String(f.size())+"}";}f=dir.openNextFile();}s.send(200,"application/json",out+"]");});
 s.on("/scripts/read",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String n=s.arg("name");if(!validName(n)){s.send(400,"text/plain","Invalid filename");return;}if(!ready){s.send(503,"text/plain","Storage unavailable");return;}File f=LittleFS.open(path(n),"r");if(!f){s.send(404,"text/plain","Not found");return;}if(f.size()>MAX_SCRIPT){f.close();s.send(413,"text/plain","File too large");return;}String body=f.readString();f.close();s.send(200,"text/plain; charset=utf-8",body);});
 s.on("/scripts/save",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String n=s.arg("name"),body=s.arg("content");if(!validName(n)){s.send(400,"text/plain","Invalid filename or extension");return;}if(body.length()>MAX_SCRIPT){s.send(413,"text/plain","8 KB limit");return;}if(!ready){s.send(503,"text/plain","Storage unavailable");return;}File f=LittleFS.open(path(n),"w");if(!f){s.send(507,"text/plain","Cannot open file");return;}size_t count=f.print(body);f.close();if(count!=body.length()){s.send(507,"text/plain","Incomplete write");return;}s.send(200,"application/json","{\"ok\":true}");});
 s.on("/scripts/delete",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String n=s.arg("name");if(!validName(n)){s.send(400,"text/plain","Invalid filename");return;}if(!ready){s.send(503,"text/plain","Storage unavailable");return;}if(!LittleFS.exists(path(n))){s.send(404,"text/plain","Not found");return;}s.send(LittleFS.remove(path(n))?200:500,"application/json",LittleFS.exists(path(n))?"{\"ok\":false}":"{\"ok\":true}");});
 s.on("/scripts/export",HTTP_GET,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String n=s.arg("name");if(!validName(n)){s.send(400,"text/plain","Invalid filename");return;}
  if(!ready){s.send(503,"text/plain","Storage unavailable");return;}
  File f=LittleFS.open(path(n),"r");if(!f){s.send(404,"text/plain","Not found");return;}
  if(f.size()>MAX_SCRIPT){f.close();s.send(413,"text/plain","File too large");return;}
  s.sendHeader("Content-Disposition",String("attachment; filename=\"")+n+"\"");
  s.streamFile(f,"application/octet-stream");f.close();
 });
 s.on("/scripts/preview",HTTP_GET,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);
  String n=s.arg("name");if(!validName(n)||!(n.endsWith(".duck")||n.endsWith(".ds")||n.endsWith(".txt"))){s.send(400,"text/plain","Unsupported script filename");return;}
  if(!ready){s.send(503,"text/plain","Storage unavailable");return;}
  File f=LittleFS.open(path(n),"r");if(!f){s.send(404,"text/plain","Not found");return;}
  if(f.size()>MAX_SCRIPT){f.close();s.send(413,"text/plain","File too large");return;}
  String body=f.readString();f.close();s.send(200,"application/json",preview(body));
 });
 s.on("/scripts",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",R"WEB(<!doctype html><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8"><title>Cyberdeck Script Manager</title><style>body{background:#091526;color:#def;font:16px sans-serif;max-width:760px;margin:auto;padding:15px}input,textarea,button{box-sizing:border-box;background:#152b41;color:white;border:1px solid #0cf;padding:10px;margin:5px 0;border-radius:5px}input,textarea{width:100%}textarea{height:240px}button{margin-right:8px}pre{white-space:pre-wrap}</style><h2>Cyberdeck Script Manager</h2><p>Store and edit scripts only. Execution is disabled.</p><input id="token" type="password" placeholder="Admin token"><input id="name" placeholder="example.duck"><textarea id="content" placeholder="Script contents"></textarea><button onclick="list()">List</button><button onclick="read()">Open</button><button onclick="save()">Save</button><button onclick="del()">Delete</button><button onclick="preview()">Preview macro</button><button onclick="exportFile()">Download</button><pre id="result"></pre><a href="/wifi">Wi-Fi manager</a><script>const el=id=>document.getElementById(id);async function call(url,post){const r=await fetch(url,{method:post?'POST':'GET',headers:{'X-Cyberdeck-Token':el('token').value,...(post?{'Content-Type':'application/x-www-form-urlencoded'}:{})},body:post?new URLSearchParams(post):undefined});const t=await r.text();el('result').textContent=(r.ok?'':'HTTP '+r.status+' ')+t;return r.ok?t:null}function list(){call('/scripts/list')}async function read(){const t=await call('/scripts/read?name='+encodeURIComponent(el('name').value));if(t!==null)el('content').value=t}function save(){call('/scripts/save',{name:el('name').value,content:el('content').value})}function preview(){call('/scripts/preview?name='+encodeURIComponent(el('name').value))}async function exportFile(){const r=await fetch('/scripts/export?name='+encodeURIComponent(el('name').value),{headers:{'X-Cyberdeck-Token':el('token').value}});if(!r.ok){el('result').textContent='Export failed: '+r.status;return}const blob=await r.blob(),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=el('name').value;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)}function del(){if(confirm('Delete '+el('name').value+'?'))call('/scripts/delete',{name:el('name').value})}</script>)WEB");});
}
}
