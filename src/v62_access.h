#pragma once
#include "v62_parameters.h"
#include "v62_categories.h"
#include "v62_icons.h"
#include "v62_wolfdeck_theme.h"

static bool v62IsPage(const String &p){
 return p=="/apps"||p=="/apps/scripts"||p.startsWith("/wolfdeck_")||p=="/"||p=="/logs"||p=="/wifi"||p=="/scripts"||p=="/toolbox"||
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
   "\",\"fields\":\""+v62Fields(p)+"\",\"icon\":\""+V62Icons::route(p.path.c_str()).id+"\",\"page\":"+(v62IsPage(p.path)?"true":"false")+",\"category\":"+String(V62Menu::category(p.path.c_str()))+"}";
 }
 return j+"]";
}
static const char V62_HOME[] PROGMEM=R"HTML(<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>WOLF DECK · Cyberdeck V6.2 Full Access</title><style>body{margin:auto;padding:18px;max-width:900px;background:#071526;color:#def;font:16px system-ui}h1,a{color:#4df}button,input,textarea,select{box-sizing:border-box;padding:12px;border:1px solid #4df;border-radius:7px;background:#132b42;color:white;margin:5px 0}input,textarea,select{width:100%}button{cursor:pointer}section{border:1px solid #245;border-radius:12px;padding:15px;margin:12px 0}pre{white-space:pre-wrap;overflow-wrap:anywhere}nav{display:flex;gap:14px;flex-wrap:wrap}nav button,nav a{display:inline-flex;align-items:center;gap:7px}.app-icon,#program-icon{flex-shrink:0;vertical-align:middle}#program-icon[hidden]{display:none}label{display:block}textarea{min-height:110px}</style><h1>WOLF DECK</h1><p>Cyberdeck V6.2 Full Access</p><p><a href="/apps">App guides and module setup</a> · <a href="/apps/scripts">BadUSB / UART scripts</a><br>All installed tools. Select a tool to continue.</p><nav id="categories" aria-label="Tool categories"></nav><h2 id="category-title">All tools</h2><nav id="pages"></nav><section><input id="search" placeholder="Find program: wifi, scripts, uart, adc, ble, sd..."><select id="program" size="8"></select><h2><img id="program-icon" width="32" height="32" alt="" hidden> <span id="title"></span></h2><div id="fields"></div><button id="run">Run selected program</button><button id="download">Download result</button><pre id="out" aria-live="polite"></pre></section><section><h2>Hardware modules</h2><p>PN532 UID reading, CC1101 packet mode, IR NEC transmit, Dallas 1-Wire and RDM6300 readers have configurable drivers. Open App guides for exact limits. Modules need external hardware and manual pin setup. USB HID and scripts start only on request.</p><button id="scanble">Scan BLE</button><button id="stop">STOP ALL</button></section><script>
const el=id=>document.getElementById(id);let apps=[],shown=[],lastBody='',busy=false,category=-1;const categories=['Network','Bluetooth','Hardware','Files / scripts','System','Settings','Radio / modules'];
function titleOf(path){const names={wifi:'Wi-Fi',uart:'UART',ble:'BLE',sd:'microSD',adc:'ADC',i2c:'I2C',usb:'USB'};return path.split('/').filter(x=>x&&!['api','v61','v2','toolbox'].includes(x)).map(x=>names[x]||x.replaceAll('-',' ').replace(/^./,c=>c.toUpperCase())).join(' · ')}
function iconOf(id){let img=document.createElement('img');img.src='/wolfdeck_icons/'+(/^[a-z0-9-]+$/.test(id||'')?id:'system')+'.svg';img.width=24;img.height=24;img.alt='';img.className='app-icon';return img}
function select(){let p=shown[el('program').selectedIndex];el('fields').replaceChildren();if(!p){el('title').textContent='No tools in this category';el('program-icon').hidden=true;return}el('title').textContent=titleOf(p.path);el('program-icon').src=iconOf(p.icon).src;el('program-icon').hidden=false;for(let name of p.fields.split(',').filter(Boolean)){let label=document.createElement('label');label.textContent=name;let input=document.createElement(name==='content'?'textarea':'input');input.name=name;const defaults={rx:'4',tx:'5',baud:'9600',pin:'1',host:'example.com',frequency:'433920000',id:'nfc',folder:'badusb'};if(defaults[name])input.value=defaults[name];if(name==='password')input.type='password';label.append(input);el('fields').append(label)}}
function chooseCategory(id){category=id;el('category-title').textContent=id<0?'All tools':categories[id];el('search').value='';for(let b of el('categories').children)b.setAttribute('aria-pressed',String(Number(b.dataset.category)===id));el('pages').replaceChildren();for(let p of apps.filter(p=>p.page&&p.path!=='/'&&(id<0||p.category===id))){let link=document.createElement('a');link.href=p.path;link.append(iconOf(p.icon),document.createTextNode(' '+titleOf(p.path)));el('pages').append(link)}filter()}
function filter(){let q=el('search').value.toLowerCase();shown=apps.filter(p=>!p.page&&(category<0||p.category===category)&&p.path.toLowerCase().includes(q)).sort((a,b)=>titleOf(a.path).localeCompare(titleOf(b.path)));el('program').replaceChildren(...shown.map(p=>{let o=document.createElement('option');o.textContent=titleOf(p.path)+(p.method==='POST'?' — action':'');return o}));el('program').selectedIndex=0;select()}
async function request(p,fields={}){if(busy)return;busy=true;el('out').textContent='Working…';try{let query=new URLSearchParams(fields),opts={method:p.method,headers:{}},url=p.path;if(p.method==='POST'){opts.headers['Content-Type']='application/x-www-form-urlencoded';opts.body=query}else if(query.size)url+='?'+query;let r=await fetch(url,opts);lastBody=await r.text();el('out').textContent='HTTP '+r.status+'\n'+lastBody}catch(e){el('out').textContent=e.message}finally{busy=false}}
el('program').onchange=select;el('search').oninput=filter;el('run').onclick=()=>{let p=shown[el('program').selectedIndex];if(!p)return;let f={};for(let i of el('fields').querySelectorAll('input,textarea'))f[i.name]=i.value;if(p.method==='POST'&&!confirm('Run '+titleOf(p.path)+'?'))return;request(p,f)};
el('stop').onclick=()=>request({path:'/apps/stop',method:'POST'});el('scanble').onclick=()=>request({path:'/access/ble',method:'POST'});
el('download').onclick=()=>{let a=document.createElement('a'),u=URL.createObjectURL(new Blob([lastBody],{type:'text/plain'}));a.href=u;a.download='cyberdeck-result.txt';a.click();setTimeout(()=>URL.revokeObjectURL(u),1000)};
fetch('/access/catalog').then(r=>r.json()).then(a=>{apps=a;for(let id=-1;id<categories.length;id++){let button=document.createElement('button');button.type='button';button.dataset.category=id;button.append(iconOf(id<0?'all-tools':['network','bluetooth','hardware','files','system','settings','radio'][id]),document.createTextNode(' '+(id<0?'All tools':categories[id])));button.onclick=()=>chooseCategory(id);el('categories').append(button)}chooseCategory(0)}).catch(e=>el('out').textContent=e.message);
</script></html>)HTML";

static void v62AccessRoutes(){
 for(const auto &icon:V62Icons::icons){
  const auto *asset=&icon;
  String path=String("/wolfdeck_icons/")+icon.id+".svg";
  v6Server.on(path.c_str(),HTTP_GET,[asset](){
   v6Server.sendHeader("Cache-Control","public, max-age=3600");
   v6Server.sendHeader("X-Content-Type-Options","nosniff");
   v6Server.send_P(200,"image/svg+xml",asset->svg,asset->length);
  });
 }

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
