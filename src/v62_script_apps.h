#pragma once
#include "v62_script_runtime.h"
#include "v61_scripts.h"
#include <esp_partition.h>
namespace V62ScriptApps {
static bool folderOK(const String &s){return s=="badusb"||s=="uart";}
static String path(const String &folder,const String &name){return String("/")+folder+"/"+name;}
static String content(const V62ScriptAssets::File &file){
 if(V61Scripts::ready){File f=LittleFS.open(path(file.folder,file.name),"r");if(f&&f.size()<=8192){String body=f.readString();f.close();return body;}}
 return String(file.content);
}
static void seed(){
 if(!V61Scripts::ready)return;
 for(const char *folder:{"badusb","uart"})if(!LittleFS.exists(String("/")+folder))LittleFS.mkdir(String("/")+folder);
 for(const auto &file:V62ScriptAssets::files){
  String p=path(file.folder,file.name);if(!LittleFS.exists(p)){File f=LittleFS.open(p,"w");if(f){size_t n=f.print(file.content);f.close();if(n!=strlen(file.content))LittleFS.remove(p);}}
  p+=".md";if(!LittleFS.exists(p)){File f=LittleFS.open(p,"w");if(f){size_t n=f.print(file.guide);f.close();if(n!=strlen(file.guide))LittleFS.remove(p);}}
 }
}
static bool initializeBlank(){
 if(V61Scripts::ready)return true;
 const esp_partition_t *p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"storage");
 if(!p)return false;uint8_t block[256];
 // Never format existing or damaged data: only completely erased partitions.
 for(size_t pos=0;pos<p->size;pos+=sizeof block){size_t n=std::min(sizeof block,size_t(p->size-pos));if((pos&0xffff)==0)delay(1);if(esp_partition_read(p,pos,block,n)!=ESP_OK)return false;for(size_t i=0;i<n;i++)if(block[i]!=0xff)return false;}
 V61Scripts::ready=LittleFS.begin(true,"/littlefs",10,"storage");if(V61Scripts::ready){LittleFS.mkdir("/scripts");seed();}return V61Scripts::ready;
}
static const char PAGE[] PROGMEM=R"PAGE(<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>WOLF DECK Script Library</title><style>body{max-width:900px;margin:auto;padding:18px;font:16px system-ui}button,input,textarea,select{padding:10px;margin:5px}textarea{width:100%;min-height:230px;box-sizing:border-box;font:13px monospace}pre{white-space:pre-wrap}#files{max-height:280px;overflow-y:scroll}#files button{display:block;width:96%;text-align:left}#guide{max-height:55vh;overflow-y:auto}</style><h1>WOLF DECK scripts</h1><p>Built-in files work without SD or initialized storage. Arduino .ino references run on a separate test board.</p><p><button id="badusb">BadUSB</button><button id="uart">UART scripts</button><button id="initialize">Initialize erased storage</button></p><div id="files"></div><h2 id="name"></h2><pre id="guide"></pre><textarea id="content" aria-label="Script contents"></textarea><p><button id="validate">Validate</button><button id="save">Save editable copy</button><button id="hid">Start native USB HID</button><button id="run">Run script</button><button id="stop">STOP</button><button id="status">Status</button><button id="download">Download script</button></p><p>Start UART first in <a href="/toolbox/uart">UART Lab</a>. USB HID needs native GPIO19/20 USB wiring. No automatic execution.</p><pre id="out" aria-live="polite"></pre><a href="/">Home</a><script>
const el=id=>document.getElementById(id);let folder='badusb',selected='',busy=false;
async function call(path,method='GET',data={}){let opts={method,headers:{}};if(method==='GET'){let q=new URLSearchParams(data);if(q.size)path+='?'+q}else{opts.headers['Content-Type']='application/x-www-form-urlencoded';opts.body=new URLSearchParams(data)}let r=await fetch(path,opts),text=await r.text();if(!r.ok)throw new Error('HTTP '+r.status+' '+text);return text}
async function list(kind){folder=kind;selected='';el('files').replaceChildren();let files=JSON.parse(await call('/apps/scripts/list','GET',{folder}));for(let file of files){let b=document.createElement('button');b.textContent=file.name+' · '+file.runtime;b.onclick=()=>open(file.name);el('files').append(b)}}
async function open(name){selected=name;el('name').textContent=name;el('content').value=await call('/apps/scripts/read','GET',{folder,name});el('guide').textContent=await call('/apps/scripts/guide','GET',{folder,name})}
async function action(op){if(busy)return;busy=true;try{if(!selected&&op!=='status'&&op!=='stop')throw Error('Select a file first');if(op==='run'&&!confirm('Run '+selected+' on your connected test device?'))return;el('out').textContent=await call('/apps/scripts/'+op,op==='status'?'GET':'POST',{folder,name:selected,content:el('content').value})}catch(e){el('out').textContent=e.message}finally{busy=false}}
for(let op of ['validate','save','run','stop','status'])el(op).onclick=()=>action(op);
for(let kind of ['badusb','uart'])el(kind).onclick=()=>list(kind).catch(e=>el('out').textContent=e.message);
el('hid').onclick=()=>call('/api/v61/usb-hid/start','POST').then(s=>el('out').textContent=s).catch(e=>el('out').textContent=e.message);
el('initialize').onclick=()=>call('/apps/storage/initialize','POST').then(s=>{el('out').textContent=s;return list(folder)}).catch(e=>el('out').textContent=e.message);
el('download').onclick=()=>{if(!selected)return;let a=document.createElement('a'),u=URL.createObjectURL(new Blob([el('content').value],{type:'text/plain'}));a.href=u;a.download=selected;a.click();setTimeout(()=>URL.revokeObjectURL(u),1000)};

list('badusb').catch(e=>el('out').textContent=e.message);
</script>)PAGE";
static void routes(V62Server &s){
 s.on("/apps/scripts",HTTP_GET,[&s](){s.send(200,"text/html; charset=utf-8",PAGE);});
 s.on("/apps/scripts/list",HTTP_GET,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),out="[";bool first=true;if(!folderOK(folder)){s.send(400,"text/plain","Folder must be badusb or uart");return;}
  for(const auto &f:V62ScriptAssets::files)if(folder==f.folder){if(!first)out+=",";first=false;out+="{\"name\":"+V62Scripts::quoted(f.name)+",\"runtime\":"+V62Scripts::quoted(f.runtime)+",\"source\":"+V62Scripts::quoted(f.source)+"}";}s.send(200,"application/json",out+"]");
 });
 s.on("/apps/scripts/read",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),name=s.arg("name");auto *f=V62Scripts::find(folder,name);if(!f){s.send(404,"text/plain","Built-in file not found");return;}s.send(200,"text/plain; charset=utf-8",content(*f));});
 s.on("/apps/scripts/guide",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),name=s.arg("name");auto *f=V62Scripts::find(folder,name);if(!f){s.send(404,"text/plain","Guide not found");return;}s.send_P(200,"text/plain; charset=utf-8",f->guide,strlen(f->guide));});
 s.on("/apps/scripts/validate",HTTP_POST,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),name=s.arg("name"),body=s.arg("content");auto *f=V62Scripts::find(folder,name);
  if(!f||String(f->runtime)=="reference-arduino"){s.send(400,"text/plain","Select a runnable .uart or BadUSB script; .ino files are reference sketches");return;}if(!body.length())body=content(*f);auto p=V62Macro::parse(body.c_str(),folder=="uart");
  s.send(p.error.empty()?200:400,"application/json",String("{\"valid\":")+(p.error.empty()?"true":"false")+",\"expanded_steps\":"+String(p.steps.size())+",\"error_line\":"+String(p.errorLine)+",\"error\":"+V62Scripts::quoted(p.error.c_str())+"}");
 });
 s.on("/apps/scripts/run",HTTP_POST,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),name=s.arg("name"),body=s.arg("content");auto *f=V62Scripts::find(folder,name);if(!f){s.send(404,"text/plain","Select a script first");return;}if(!body.length())body=content(*f);
  V62ScriptAssets::File copy=*f;copy.content=body.c_str();bool ok=V62Scripts::start(copy);s.send(ok?200:409,"application/json",V62Scripts::state());
 });
 s.on("/apps/scripts/save",HTTP_POST,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String folder=s.arg("folder"),name=s.arg("name"),body=s.arg("content");auto *f=V62Scripts::find(folder,name);if(!f||body.length()>8192){s.send(400,"text/plain","Unknown file or 8 KiB limit exceeded");return;}if(!V61Scripts::ready){s.send(503,"text/plain","Editable storage unavailable. Initialize only an erased partition; built-ins remain usable.");return;}
  File out=LittleFS.open(path(folder,name),"w");if(!out){s.send(507,"text/plain","Cannot save file");return;}size_t n=out.print(body);out.close();s.send(n==body.length()?200:507,"text/plain",n==body.length()?"Editable copy saved":"Incomplete write");
 });
 s.on("/apps/scripts/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",V62Scripts::state());});
 s.on("/apps/scripts/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);V62Scripts::stop();s.send(200,"application/json",V62Scripts::state());});
 s.on("/apps/storage/initialize",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);if(V62Scripts::running){s.send(409,"text/plain","Stop scripts before initializing storage");return;}bool ok=initializeBlank();s.send(ok?200:409,"text/plain",ok?"Storage ready; built-in badusb and uart folders installed without overwriting files":"Not formatted: partition contains data, is missing or initialization failed. Existing data preserved; use built-in library.");});
}
}
