#pragma once
#include "v62_parameters.h"
#include "v62_wolfdeck_theme.h"

static bool v62IsPage(const String &p){
 return p.startsWith("/wolfdeck_")||p=="/"||p=="/logs"||p=="/wifi"||p=="/scripts"||p=="/toolbox"||
 (p.startsWith("/toolbox/")&&p!="/toolbox/health"&&p!="/toolbox/wifi"&&
 p!="/toolbox/memory"&&p!="/toolbox/flash"&&p!="/toolbox/storage"&&
 p!="/toolbox/i2c"&&p!="/toolbox/pins"&&p!="/toolbox/features"&&
 p!="/toolbox/clock"&&p!="/toolbox/help");
}
static String v62ProgramName(String path){
 path.replace("/api/v61/v2/","");path.replace("/api/v61/","");
 path.replace("/toolbox/","");if(path.startsWith("/"))path.remove(0,1);
 path.replace("/"," > ");path.replace("-"," ");return path;
}
static String v62Fields(const V62Server::Entry &entry){
 for(const auto &p:v62Parameters)if(entry.path==p.path&&entry.method==p.method)return p.fields;
 return "";
}
static String v62Catalog(){
 String j="[";bool first=true;
 for(const auto &p:v6Server.programs){
  if(p.path.startsWith("/access")||p.path.startsWith("/wolfdeck_"))continue;
  if(!first)j+=",";
  first=false;
  j+="{\"path\":\""+p.path+"\",\"method\":\""+(p.method==HTTP_POST?"POST":"GET")+
   "\",\"fields\":\""+v62Fields(p)+"\",\"page\":"+(v62IsPage(p.path)?"true":"false")+"}";
 }
 return j+"]";
}
static const char V62_HOME[] PROGMEM=R"HTML(<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>WOLF DECK · Cyberdeck V6.2 Full Access</title><style>body{margin:auto;padding:18px;max-width:900px;background:#071526;color:#def;font:16px system-ui}h1,a{color:#4df}button,input,textarea,select{box-sizing:border-box;padding:12px;border:1px solid #4df;border-radius:7px;background:#132b42;color:white;margin:5px 0}input,textarea,select{width:100%}button{cursor:pointer}section{border:1px solid #245;border-radius:12px;padding:15px;margin:12px 0}pre{white-space:pre-wrap;overflow-wrap:anywhere}nav{display:flex;gap:14px;flex-wrap:wrap}label{display:block}textarea{min-height:110px}</style><h1>WOLF DECK</h1><p>Cyberdeck V6.2 Full Access</p><p>All installed tools. Enter the admin token from Settings to use protected actions.</p><input id="token" type="password" autocomplete="off" placeholder="Admin token"><nav id="pages"></nav><section><input id="search" placeholder="Find program: wifi, scripts, uart, adc, ble, sd..."><select id="program" size="8"></select><h2 id="title"></h2><div id="fields"></div><button id="run">Run selected program</button><button id="download">Download result</button><pre id="out" aria-live="polite"></pre></section><section><h2>Hardware modules</h2><p>NFC/PN532, CC1101, IR and GPIO output are not configured in the V6.1 source. USB HID retains its existing build gate. UART, ADC, microSD and BLE receiver are manually controlled using the programs above.</p><button id="scanble">Scan BLE</button><button id="stop">STOP BLE + UART + HID</button></section><script>
const el=id=>document.getElementById(id);let apps=[],shown=[],lastBody='',busy=false;
function titleOf(path){const names={wifi:'Wi-Fi',uart:'UART',ble:'BLE',sd:'microSD',adc:'ADC',i2c:'I2C',usb:'USB'};return path.split('/').filter(x=>x&&!['api','v61','v2','toolbox'].includes(x)).map(x=>names[x]||x.replaceAll('-',' ').replace(/^./,c=>c.toUpperCase())).join(' · ')}
function select(){let p=shown[el('program').selectedIndex];el('fields').replaceChildren();if(!p)return;el('title').textContent=titleOf(p.path);for(let name of p.fields.split(',').filter(Boolean)){let label=document.createElement('label');label.textContent=name;let input=document.createElement(name==='content'?'textarea':'input');input.name=name;if(name==='password')input.type='password';label.append(input);el('fields').append(label)}}
function filter(){let q=el('search').value.toLowerCase();shown=apps.filter(p=>!p.page&&p.path.toLowerCase().includes(q));el('program').replaceChildren(...shown.map(p=>{let o=document.createElement('option');o.textContent=titleOf(p.path)+(p.method==='POST'?' — action':'');return o}));el('program').selectedIndex=0;select()}
async function request(p,fields={}){if(busy)return;busy=true;el('out').textContent='Working…';try{let query=new URLSearchParams(fields),opts={method:p.method,headers:{'X-Cyberdeck-Token':el('token').value}},url=p.path;if(p.method==='POST'){opts.headers['Content-Type']='application/x-www-form-urlencoded';opts.body=query}else if(query.size)url+='?'+query;let r=await fetch(url,opts);lastBody=await r.text();el('out').textContent='HTTP '+r.status+'\n'+lastBody}catch(e){el('out').textContent=e.message}finally{busy=false}}
el('program').onchange=select;el('search').oninput=filter;el('run').onclick=()=>{let p=shown[el('program').selectedIndex];if(!p)return;let f={};for(let i of el('fields').querySelectorAll('input,textarea'))f[i.name]=i.value;if(p.method==='POST'&&!confirm('Run '+titleOf(p.path)+'?'))return;request(p,f)};
el('stop').onclick=()=>request({path:'/api/v61/release/stop',method:'POST'});el('scanble').onclick=()=>request({path:'/access/ble',method:'POST'});
el('download').onclick=()=>{let a=document.createElement('a'),u=URL.createObjectURL(new Blob([lastBody],{type:'text/plain'}));a.href=u;a.download='cyberdeck-result.txt';a.click();setTimeout(()=>URL.revokeObjectURL(u),1000)};
fetch('/access/catalog').then(r=>r.json()).then(a=>{apps=a;for(let p of a.filter(p=>p.page&&p.path!=='/')){let link=document.createElement('a');link.href=p.path;link.textContent=titleOf(p.path);el('pages').append(link)}filter()}).catch(e=>el('out').textContent=e.message);
</script></html>)HTML";

static void v62AccessRoutes(){
 // WebServer resolves the first registered route: update root registration in
 // v6Start rather than adding a second root handler here.
 v6Server.on("/wolfdeck_web_background.jpg",HTTP_GET,[](){
  v6Server.sendHeader("Cache-Control","public, max-age=86400");
  v6Server.send_P(200,"image/jpeg",reinterpret_cast<const char*>(WolfDeckAssets::webJpg),sizeof(WolfDeckAssets::webJpg));
 });
 v6Server.on("/wolfdeck_theme.css",HTTP_GET,[](){
  v6Server.sendHeader("Cache-Control","public, max-age=3600");
  v6Server.send_P(200,"text/css; charset=utf-8",WOLFDECK_THEME,sizeof(WOLFDECK_THEME)-1);
 });
 v6Server.on("/access",HTTP_GET,[](){v6Server.send(200,"text/html; charset=utf-8",V62_HOME);});
 v6Server.on("/access/catalog",HTTP_GET,[](){v6Server.send(200,"application/json",v62Catalog());});
}
