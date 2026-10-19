// UI logic test with a mocked device transport; this is not a hardware test.
const {JSDOM,VirtualConsole}=require(process.env.V62_JSDOM||'jsdom');
const fs=require('node:fs'),assert=require('node:assert/strict');
const path=require('node:path'),root=path.resolve(__dirname,'..');
const html=fs.readFileSync(path.join(root,'src/v62_access.h'),'utf8').match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];
const metadata=fs.readFileSync(path.join(root,'src/v62_parameters.h'),'utf8');
const catalog=[...metadata.matchAll(/\{"([^"]+)",HTTP_(GET|POST),"([^"]*)"\}/g)].map(m=>({path:m[1],method:m[2],fields:m[3],page:m[1]==='/wifi'||m[1]==='/scripts'}));
for(let p of catalog){p.category=p.path.includes('ble')?1:p.path.includes('scripts')||p.path.includes('/sd/')||p.path.includes('storage')?3:p.path.includes('uart')||p.path.includes('adc')||p.path.includes('i2c')||p.path.includes('pins')||p.path.includes('hardware/plan')||p.path.includes('usb-hid')?2:['/wifi/ap','/wifi/networks','/wifi/delete'].includes(p.path)?5:p.path.includes('wifi')||p.path.includes('network')||p.path.includes('/scan/')?0:4}
let requests=[],confirm=true,errors=[];
const console=new VirtualConsole();console.on('jsdomError',e=>errors.push(e.message));
const dom=new JSDOM(html,{url:'http://cyberdeck.test/access',runScripts:'dangerously',virtualConsole:console,beforeParse(w){
 w.URLSearchParams=URLSearchParams;w.confirm=()=>confirm;
 w.fetch=async(url,opts)=>{
  if(url==='/access/catalog')return {json:async()=>catalog};
  requests.push({url,...opts});return {status:200,text:async()=>'mock-device-result'};
 };
}});
const w=dom.window,el=id=>w.document.getElementById(id),tick=()=>new Promise(resolve=>setTimeout(resolve,0));
function search(q){el('search').value=q;el('search').dispatchEvent(new w.Event('input'));}
function field(name,value){w.document.querySelector(`[name="${name}"]`).value=value;}
(async()=>{
 await tick();assert.equal(el('category-title').textContent,'Network');assert(el('program').options.length>0);
 el('categories').querySelector('[data-category="-1"]').click();assert(el('program').options.length>70);
 assert.equal(el('categories').children.length,8);
 el('categories').querySelector('[data-category="2"]').click();assert.equal(el('category-title').textContent,'Hardware');
 const theme=fs.readFileSync(path.join(root,'src/v62_wolfdeck_theme.h'),'utf8').match(/R"CSS\(([\s\S]*?)\)CSS"/)[1];
 const style=w.document.createElement('style');style.textContent=theme;w.document.head.append(style);
 assert(w.getComputedStyle(w.document.body).backgroundImage.includes('wolfdeck_web_background.jpg'));
 assert.equal(w.getComputedStyle(w.document.body).color,'rgb(237, 248, 255)');
 assert.equal(w.document.querySelector('h1').textContent,'WOLF DECK');
 el('token').value='private-test-token';search('/api/v61/uart/start');
 field('rx','4');field('tx','5');field('baud','4800');el('run').click();await tick();
 let r=requests.at(-1);assert.equal(r.url,'/api/v61/uart/start');assert.equal(r.method,'POST');
 assert.equal(r.headers['X-Cyberdeck-Token'],'private-test-token');assert.equal(r.body.toString(),'rx=4&tx=5&baud=4800');
 assert.equal(el('out').textContent,'HTTP 200\nmock-device-result');
 el('categories').querySelector('[data-category="3"]').click();search('/scripts/save');field('name','test.txt');field('content','line 1\n<&line 2');el('run').click();await tick();
 assert.equal(requests.at(-1).body.get('content'),'line 1\n<&line 2');
 confirm=false;let count=requests.length;el('run').click();await tick();assert.equal(requests.length,count);
 el('categories').querySelector('[data-category="0"]').click();search('/wifi/dns');field('host','example.test');el('run').click();await tick();assert.equal(requests.at(-1).url,'/wifi/dns?host=example.test');
 search('no-such-tool');el('run').click();await tick();assert.equal(requests.length,count+1);
 assert.deepEqual(errors,[]);dom.window.close();
 process.stdout.write('PASS: catalog, filter, UART fields, token header, POST confirmation, multiline text, GET encoding, empty results\n');
})().catch(e=>{process.stderr.write(e.stack+'\n');process.exitCode=1;dom.window.close()});
