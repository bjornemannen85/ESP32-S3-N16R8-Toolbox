#!/usr/bin/env python3
"""Build original WOLF DECK vector icons, PNG exports and packed TFT masks."""
from pathlib import Path
import json,re,io,hashlib
import cairosvg
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/wolfdeck/icons'
# Original geometric marks, on a consistent 64-unit grid. No external artwork.
ICONS={
 'network':('Network','network','M12 19h14v12H12z M38 19h14v12H38z M25 43h14v12H25z M19 31v6h26v-6 M32 37v6'),
 'wifi':('WiFi scanner','network','M9 24Q32 5 55 24 M16 33Q32 19 48 33 M24 42Q32 34 40 42 M31 51h2'),
 'nat':('NAT router','network','M10 37h44v15H10z M19 37V20 M45 37V20 M14 18l5-6 5 6 M40 18l5-6 5 6 M18 44h1 M26 44h1 M39 44h8'),
 'dns':('DNS lookup','network','M11 15h42v34H11z M11 25h42 M19 20h1 M25 20h1 M29 33a8 8 0 1 0 0 16a8 8 0 1 0 0-16 M35 46l9 8'),
 'http':('HTTP client','network','M12 13h40v38H12z M12 23h40 M19 18h1 M25 18h1 M26 31l-6 6 6 6 M38 31l6 6-6 6 M35 29l-6 16'),
 'tcp':('TCP port test','network','M10 21h14v22H10z M40 21h14v22H40z M24 28h16 M24 36h16 M29 23l5 5-5 5 M35 31l-5 5 5 5'),
 'history':('Scan history','network','M15 21a21 21 0 1 1-3 17 M8 12v14h14 M32 19v14l10 6'),
 'bluetooth':('BLE scanner','bluetooth','M30 9v46l16-14-28-20 M30 9l16 14-28 20'),
 'ble-transfer':('BLE file receiver','bluetooth','M22 10v35l12-10L12 19 M22 10l12 11-22 16 M40 42h15 M49 36l6 6-6 6 M38 51h16'),
 'hardware':('Hardware tools','hardware','M20 20h24v24H20z M26 26h12v12H26z M25 11v9 M33 11v9 M41 11v9 M25 44v9 M33 44v9 M41 44v9 M11 25h9 M11 33h9 M44 25h9 M44 33h9'),
 'uart':('UART terminal','hardware','M10 14h44v36H10z M18 24l8 8-8 8 M32 40h13'),
 'adc':('ADC meter','hardware','M10 13v38h45 M13 37h7l6-20 11 28 7-18h9'),
 'gpio':('GPIO lab','hardware','M13 13h38v38H13z M23 22v20 M32 22v20 M41 22v20 M19 27h8 M28 36h8 M37 29h8'),
 'i2c':('I2C explorer','hardware','M9 15h46 M9 49h46 M19 15v11 M45 15v11 M19 38v11 M45 38v11 M12 26h14v12H12z M38 26h14v12H38z M26 32h12'),
 'spi':('SPI diagnostics','hardware','M10 18h44 M10 32h44 M10 46h44 M19 13l6 5-6 5 M19 27l6 5-6 5 M45 41l-6 5 6 5'),
 'usb':('USB HID','hardware','M32 52V13 M27 19l5-7 5 7 M32 39l-13-9V20 M16 20h6 M32 32l13-8V15 M42 12h6v6h-6z M29 49h6v6h-6z'),
 'files':('Files / storage','files','M10 20h18l5 6h21v26H10z M10 20v-7h17l6 7h21v6'),
 'scripts':('Script manager','files','M16 10h23l10 10v34H16z M38 10v12h11 M28 30l-6 6 6 6 M38 30l6 6-6 6'),
 'backup':('Backup / export','files','M13 25h38v28H13z M13 25l6-10h26l6 10 M32 43V21 M25 28l7-7 7 7'),
 'sd':('microSD','files','M20 10h27v44H17V24l3-4z M25 12v10 M32 12v10 M39 12v10 M24 35h16v11H24z'),
 'system':('System monitor','system','M9 14h46v32H9z M23 54h18 M32 46v8 M15 33h8l5-11 8 17 5-9h8'),
 'health':('Device health','system','M12 16h40v34H12z M18 32h8l4-9 6 18 5-9h5'),
 'memory':('Heap / PSRAM','system','M9 21h46v24H9z M17 28h7v10h-7z M30 28h7v10h-7z M43 28h5v10h-5z M15 45v7 M24 45v7 M33 45v7 M42 45v7 M51 45v7'),
 'flash':('Flash information','system','M16 13h32v38H16z M22 9v4 M30 9v4 M38 9v4 M22 51v5 M30 51v5 M38 51v5 M34 20l-9 15h8l-3 10 10-16h-8z'),
 'clock':('Clock / uptime','system','M32 10a22 22 0 1 0 0 44a22 22 0 1 0 0-44 M32 18v15l11 7'),
 'logs':('Event log','system','M16 10h32v44H16z M23 21h18 M23 30h18 M23 39h12 M23 47h18'),
 'touch':('Touch test','system','M10 11h28v38H10z M30 43V26q0-5 5-5t5 5v9l4-3 9 6-5 16H35l-9-10q-2-5 4-5 M17 18h12'),
 'settings':('Settings','settings','M23 10h18l2 9 8 4v18l-8 4-2 9H23l-2-9-8-4V23l8-4z M32 23a9 9 0 1 0 0 18a9 9 0 1 0 0-18'),
 'wifi-setup':('WiFi setup','settings','M10 19Q28 5 46 19 M17 28Q28 18 39 28 M24 37h7 M38 38h17v16H38z M42 38v-5a5 5 0 0 1 10 0v5'),
 'profiles':('WiFi profiles','settings','M13 12h38v12H13z M13 28h38v12H13z M13 44h38v12H13z M20 18h1 M20 34h1 M20 50h1 M29 18h15 M29 34h15 M29 50h15'),
 'radio':('Radio tools','modules','M32 30v23 M23 53h18 M26 25a9 9 0 0 1 12 0 M18 19a20 20 0 0 1 28 0 M10 12a31 31 0 0 1 44 0 M30 31h4'),
 'nfc':('NFC / PN532','modules','M11 16h24v36H11z M19 24h8v20h-8z M41 26q7 8 0 16 M48 19q13 15 0 30'),
 'subghz':('CC1101 / Sub-GHz','modules','M18 27h28v25H18z M24 35h16v9H24z M32 27V12 M24 18q8-10 16 0 M18 12q14-14 28 0'),
 'ir':('IR analyzer','modules','M12 31l8-8 16 16-8 8z M27 23l6-6 M35 31l6-6 M36 12q15 0 15 16 M41 6q17 0 17 20'),
 'modules':('Module manager','modules','M12 12h16v16H12z M36 12h16v16H36z M12 36h16v16H12z M36 36h16v16H36z M28 20h8 M20 28v8 M44 28v8 M28 44h8'),
 'all-tools':('All tools','system','M11 11h16v16H11z M37 11h16v16H37z M11 37h16v16H11z M37 37h16v16H37z'),
 'features':('Capabilities / roadmap','system','M17 10v44 M18 14h31l-7 10 7 10H18 M10 54h17'),
 'safety':('Safety / stop','system','M23 10h18l13 13v18L41 54H23L10 41V23z M25 25h14v14H25z'),
 'help':('Help / guide','system','M23 23a10 10 0 1 1 16 8q-7 3-7 9 M31 49h2 M32 9a23 23 0 1 0 0 46a23 23 0 1 0 0-46'),
 'release':('Release information','system','M14 15h36v38H14z M22 10v10 M42 10v10 M14 25h36 M23 38l6 6 13-13'),
 'operations':('Operations','system','M12 14h17v13H12z M35 37h17v13H35z M29 20h14v17 M38 31l5 6 5-6 M20 27v17h15'),
}
COLORS={'network':'#26DDF2','bluetooth':'#718AFF','hardware':'#FFC457','files':'#57E4AD','system':'#62B7FF','settings':'#C390FF','modules':'#FF875B'}
def resolve(p):
 for needles,key in [(['/ble-transfer'],'ble-transfer'),(['/ble','bluetooth'],'bluetooth'),(['/wifi/ap'],'wifi-setup'),(['/wifi/networks','/wifi/delete'],'profiles'),(['/wifi/nat'],'nat'),(['/wifi/dns'],'dns'),(['/wifi/http'],'http'),(['/wifi/tcp'],'tcp'),(['/history','/snapshot','/scan/'],'history'),(['wifi'],'wifi'),(['uart'],'uart'),(['usb-hid'],'usb'),(['adc'],'adc'),(['i2c'],'i2c'),(['spi'],'spi'),(['pins','/io'],'gpio'),(['/sd'],'sd'),(['backup','export'],'backup'),(['scripts'],'scripts'),(['memory'],'memory'),(['flash'],'flash'),(['storage'],'files'),(['clock','runtime'],'clock'),(['health'],'health'),(['safety','/stop'],'safety'),(['release'],'release'),(['features','roadmap'],'features'),(['help'],'help'),(['operations'],'operations'),(['network'],'network'),(['hardware/plan'],'hardware')]:
  if any(n in p for n in needles):return key
 return 'system'
def main():
 OUT.mkdir(parents=True,exist_ok=True);manifest={'version':1,'style':'WOLF DECK vector marks, 64-unit grid, 3-unit strokes','icons':[],'routes':{},'category_icons':['network','bluetooth','hardware','files','system','settings','radio'],'home_icons':['network','bluetooth','hardware','files','system','settings','radio','all-tools'],'native_pages':{'HOME':'all-tools','RADIO':'radio','HARDWARE':'hardware','TOOLS':'all-tools','WIFI':'wifi','BLE':'bluetooth','SYSTEM':'system','TOUCH':'touch','NFC':'nfc','SUBGHZ':'subghz','IR':'ir','PAGE_GPIO':'gpio','UART':'uart','I2C':'i2c','SPI_DIAG':'spi','STORAGE':'sd','SETTINGS':'settings','LOGS':'logs','MODULES':'modules','WIFI_SETUP':'wifi-setup','WIFI_KEYBOARD':'wifi-setup','PROGRAMS':'all-tools','PROGRAM_ACTION':'all-tools','PROGRAM_RESULT':'all-tools','NETWORK_TOOLS':'network','BLUETOOTH_TOOLS':'bluetooth','FILE_TOOLS':'files','SYSTEM_TOOLS':'system','SETTINGS_TOOLS':'settings'}}
 masks={size:[] for size in [24,32]}
 for key,(label,category,path) in ICONS.items():
  color=COLORS[category]
  svg=f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" role="img" aria-label="{label}"><path d="{path}" fill="none" stroke="{color}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/></svg>\n'
  dest=OUT/'svg';dest.mkdir(exist_ok=True);(dest/(key+'.svg')).write_text(svg)
  for size in [16,24,32,48,64,128]:
   dest=OUT/f'png-{size}';dest.mkdir(exist_ok=True)
   data=cairosvg.svg2png(bytestring=svg.encode(),output_width=size,output_height=size)
   (dest/(key+'.png')).write_bytes(data)
   if size in masks:
    alpha=Image.open(io.BytesIO(data)).getchannel('A');packed=bytearray((size*size+7)//8)
    for i,v in enumerate(alpha.get_flattened_data()):
     if v>=96:packed[i//8]|=128>>(i%8)
    masks[size].append((key,bytes(packed)))
  manifest['icons'].append({'id':key,'label':label,'category':category,'color':color,'svg':f'svg/{key}.svg'})
 for p in re.findall(r'\{"([^"]+)"', (ROOT/'src/v62_parameters.h').read_text()):manifest['routes'][p]=resolve(p)
 (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 for size,items in masks.items():
  text='#pragma once\n#include <Arduino.h>\nnamespace WolfDeckIcons'+str(size)+' {\n'
  for key,data in items:text+='static const uint8_t '+key.replace('-','_')+'[] PROGMEM={'+','.join('0x%02x'%v for v in data)+'};\n'
  text+='}\n';(OUT/f'wolfdeck_icons_{size}.h').write_text(text)
 preview(manifest)
 files=[p for p in OUT.rglob('*') if p.is_file() and p.name!='SHA256SUMS.txt']
 (OUT/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(OUT))+'\n' for p in sorted(files)))
 print(f'{len(ICONS)} icons; {len(manifest["routes"])} routes; TFT masks: 24px={len(ICONS)*72} bytes, 32px={len(ICONS)*128} bytes')
def preview(m):
 font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
 title=ImageFont.truetype(font,38);body=ImageFont.truetype(font,16);small=ImageFont.truetype(font,13)
 cols=6;w=1200;h=170+((len(ICONS)+cols-1)//cols)*162
 im=Image.new('RGB',(w,h),'#06101e');draw=ImageDraw.Draw(im)
 draw.text((32,25),'WOLF DECK',font=title,fill='#dffaff');draw.text((33,77),'APP ICONS  /  V6.2 FULL ACCESS',font=body,fill='#26ddf2')
 draw.text((33,111),'Original vector symbols · touch 24 / 32 px · web SVG / PNG',font=body,fill='#90a8bf')
 for i,entry in enumerate(m['icons']):
  x=24+(i%cols)*196;y=160+(i//cols)*162
  draw.rounded_rectangle((x,y,x+182,y+148),radius=12,fill='#0b1c30',outline='#20394e')
  icon=Image.open(OUT/'png-128'/(entry['id']+'.png'));icon.thumbnail((72,72));im.paste(icon,(x+55,y+8),icon)
  label=entry['label'];label=label if len(label)<23 else label[:21]+'…'
  draw.text((x+91,y+91),label,font=small,fill='#e2efff',anchor='mm')
  # Native sizes are shown without enlargement beside each other.
  for size,dx in [(24,54),(32,97)]:
   icon=Image.open(OUT/f'png-{size}'/(entry['id']+'.png'));im.paste(icon,(x+dx,y+110),icon)
 im.save(OUT/'WOLF-DECK-App-Icons.png')
if __name__=='__main__':main()
