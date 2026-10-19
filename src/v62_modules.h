#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "v62_module_core.h"
#include "v61_wifi.h"
namespace V62Modules {
struct Config{const char *id;size_t count;std::vector<int> pins;};
static Config configs[]={{"nfc",4,{}},{"subghz",4,{}},{"ir",2,{}},{"ibutton",1,{}},{"lf_rfid",2,{}},{"gpio",1,{}},{"spi",4,{}},{"i2c",2,{}}};
static TwoWire externalWire(1);
static Adafruit_PN532 *nfc=nullptr;
static SPIClass radioSPI(HSPI);
static HardwareSerial lfSerial(2);
static bool radioReady=false,nfcReady=false,lfReady=false,spiReady=false,i2cReady=false;
static uint32_t radioHz=433920000;
static String last="Not configured. Configure pins, then probe.";
static bool pinFree(int p){for(const auto &c:configs)for(int n:c.pins)if(p==n)return false;return true;}
static Config *find(const String &id){for(auto &c:configs)if(id==c.id)return &c;return nullptr;}
static String q(const String &s){return String("\"")+V61WiFi::escapeJson(s)+"\"";}
static String bytes(const uint8_t *data,size_t n){String s;char b[3];for(size_t i=0;i<n;i++){snprintf(b,sizeof b,"%02X",data[i]);s+=b;}return s;}
static bool validConfig(Config &target,const String &value){
 std::vector<int> pins;if(!V62ModuleCore::pins(value.c_str(),pins,target.count)){last="Invalid pin list. Reserved LCD/touch/USB/SD/console pins are blocked.";return false;}
 for(const auto &c:configs)if(&c!=&target)for(int p:c.pins)for(int n:pins)if(n==p){last="Pin already assigned to another module";return false;}
 for(int p:pins)if(V61Bundle81100::active&&(p==V61Bundle81100::rxPin||p==V61Bundle81100::txPin)){last="Pin used by active UART";return false;}
 target.pins=pins;return true;
}
static void load(){V61Bundle81100::modulePinFree=pinFree;V61Bundle101120::modulePinFree=pinFree;Preferences p;p.begin("wdmodules",true);radioHz=p.getUInt("frequency",433920000);uint8_t f[3];if(!V62ModuleCore::frequency(radioHz,f))radioHz=433920000;for(auto &c:configs){String value=p.getString(c.id,"");if(value.length())validConfig(c,value);}p.end();last="Saved wiring loaded; all modules remain idle until requested.";}
static bool radioSelect(){auto &c=*find("subghz");digitalWrite(c.pins[3],LOW);unsigned long start=micros();while(digitalRead(c.pins[1])){if(micros()-start>3000){digitalWrite(c.pins[3],HIGH);last="CC1101 MISO timeout";return false;}}return true;}
static uint8_t radioReg(uint8_t addr,uint8_t value,bool read=false){
 auto &c=*find("subghz");radioSPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));uint8_t result=0xff;
 if(radioSelect()){radioSPI.transfer(addr|(read?0xc0:0));result=radioSPI.transfer(value);digitalWrite(c.pins[3],HIGH);}radioSPI.endTransaction();return result;
}
static void strobe(uint8_t value){auto &c=*find("subghz");radioSPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));if(radioSelect()){radioSPI.transfer(value);digitalWrite(c.pins[3],HIGH);}radioSPI.endTransaction();}
static String probe(const String &id){
 auto *c=find(id);if(!c||c->pins.size()!=c->count)return "Configure pins first";
 if(id=="nfc"){
  i2cReady=false;if(nfc){delete nfc;nfc=nullptr;}externalWire.end();externalWire.begin(c->pins[0],c->pins[1],100000);
  nfc=new Adafruit_PN532(c->pins[2],c->pins[3],&externalWire);nfc->begin();uint32_t version=nfc->getFirmwareVersion();nfcReady=version&&nfc->SAMConfig();
  return nfcReady?String("PN532 detected, firmware ")+String((version>>16)&255)+"."+String((version>>8)&255):"PN532 not detected; check I2C mode and wiring";
 }
 if(id=="subghz"){
  if(radioReady)strobe(0x36);radioReady=false;spiReady=false;radioSPI.end();radioSPI.begin(c->pins[0],c->pins[1],c->pins[2],c->pins[3]);pinMode(c->pins[3],OUTPUT);digitalWrite(c->pins[3],HIGH);strobe(0x30);delay(5);
  uint8_t version=radioReg(0x31,0,true);radioReady=version!=0&&version!=0xff;
  if(!radioReady)return "CC1101 not detected";
  // Packet-mode 2-FSK baseline, 38.4 kbps, CRC, whitening and variable packet size.
  const uint8_t config[][2]={{0x00,0x2e},{0x02,0x2e},{0x06,61},{0x07,0x04},{0x08,0x45},{0x0b,0x06},{0x10,0xca},{0x11,0x83},{0x12,0x13},{0x13,0x22},{0x14,0xf8},{0x15,0x35},{0x18,0x18},{0x19,0x16},{0x1b,0x43},{0x1c,0x40},{0x1d,0x91},{0x21,0x56},{0x22,0x10},{0x23,0xe9},{0x24,0x2a},{0x25,0x00},{0x26,0x1f}};
  for(auto &r:config)radioReg(r[0],r[1]);uint8_t f[3];V62ModuleCore::frequency(radioHz,f);for(int i=0;i<3;i++)radioReg(0x0d+i,f[i]);strobe(0x36);
  return String("CC1101 detected, version ")+version+". Packet mode configured; idle.";
 }
 if(id=="lf_rfid"){lfSerial.end();lfSerial.begin(9600,SERIAL_8N1,c->pins[0],c->pins[1]);lfReady=true;return "9600-baud RDM6300 UART opened; present a tag to read";}
 if(id=="ir"){pinMode(c->pins[0],INPUT);pinMode(c->pins[1],OUTPUT);digitalWrite(c->pins[1],LOW);return "IR pins prepared: demodulating receiver RX, transistor-driven LED TX";}
 if(id=="gpio"){pinMode(c->pins[0],INPUT);return "GPIO input ready; Transmit 0/1 enables explicit output";}
 if(id=="i2c"){nfcReady=false;externalWire.end();i2cReady=externalWire.begin(c->pins[0],c->pins[1],100000);return i2cReady?"External I2C bus ready; Read scans addresses":"External I2C setup failed";}
 if(id=="spi"){if(radioReady)strobe(0x36);radioReady=false;radioSPI.end();radioSPI.begin(c->pins[0],c->pins[1],c->pins[2],c->pins[3]);pinMode(c->pins[3],OUTPUT);digitalWrite(c->pins[3],HIGH);spiReady=true;return "SPI mode 0, 1 MHz; Transmit HEX returns received bytes";}
 if(id=="ibutton"){pinMode(c->pins[0],INPUT_PULLUP);return "1-Wire configured; external 4.7k pull-up required";}
 return "Unsupported module";
}
static bool oneReset(int pin){pinMode(pin,OUTPUT_OPEN_DRAIN);digitalWrite(pin,LOW);delayMicroseconds(480);digitalWrite(pin,HIGH);delayMicroseconds(70);bool present=!digitalRead(pin);delayMicroseconds(410);return present;}
static void oneBit(int pin,bool bit){digitalWrite(pin,LOW);delayMicroseconds(bit?6:60);digitalWrite(pin,HIGH);delayMicroseconds(bit?64:10);}
static uint8_t oneByte(int pin){uint8_t value=0;for(int i=0;i<8;i++){digitalWrite(pin,LOW);delayMicroseconds(3);digitalWrite(pin,HIGH);delayMicroseconds(10);if(digitalRead(pin))value|=1<<i;delayMicroseconds(53);}return value;}
static String read(const String &id){
 auto *c=find(id);if(!c||c->pins.size()!=c->count)return "Configure pins first";
 if(id=="nfc"){
  if(!nfcReady)return "Probe PN532 first";uint8_t uid[10],len=0;
  if(!nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A,uid,&len,300))return "No ISO14443A tag detected";
  return String("ISO14443A UID: ")+bytes(uid,len)+" (UID read only; not card emulation)";
 }
 if(id=="ibutton"){
  int pin=c->pins[0];if(!oneReset(pin))return "No 1-Wire presence pulse";
  uint8_t cmd=0x33;for(int i=0;i<8;i++)oneBit(pin,(cmd>>i)&1);uint8_t rom[8];for(auto &b:rom)b=oneByte(pin);pinMode(pin,INPUT_PULLUP);
  if(V62ModuleCore::crc8(rom,7)!=rom[7])return "ROM CRC failed; check pull-up, wiring or bus with multiple devices";
  return String("Dallas 1-Wire ROM: ")+bytes(rom,8);
 }
 if(id=="lf_rfid"){
  if(!lfReady)return "Probe UART reader first";String frame;unsigned long start=millis();bool active=false;
  while(millis()-start<300){while(lfSerial.available()){int ch=lfSerial.read();if(ch==2){frame="";active=true;}else if(active&&ch==3){std::vector<uint8_t> data;if(!V62ModuleCore::hex(frame.c_str(),data,6)||data.size()!=6)return "Invalid RDM6300 frame";uint8_t sum=0;for(int i=0;i<5;i++)sum^=data[i];return sum==data[5]?String("EM4100 ID: ")+bytes(data.data(),5):"RFID checksum failed";}else if(active&&frame.length()<12)frame+=char(ch);}delay(1);}
  return "No RDM6300 tag frame";
 }
 if(id=="subghz"){
  if(!radioReady)return "Probe CC1101 first";strobe(0x34);delay(10);int8_t raw=int8_t(radioReg(0x34,0,true));uint8_t count=radioReg(0x3b,0,true);
  String out=String("RSSI: ")+String(float(raw)/2-74,1)+" dBm; RX bytes: "+String(count&0x7f);
  if(count&0x80){strobe(0x36);strobe(0x3a);out+="; RX overflow cleared";}
  else if((count&0x7f)>0){uint8_t n=radioReg(0x3f,0,true);if(n<=61&&n+3<=(count&0x7f)){uint8_t data[61];for(int i=0;i<n;i++)data[i]=radioReg(0x3f,0,true);uint8_t rssi=radioReg(0x3f,0,true),lqi=radioReg(0x3f,0,true);(void)rssi;out+="; packet="+bytes(data,n)+"; CRC="+String((lqi&0x80)?"ok":"failed");}else{strobe(0x36);strobe(0x3a);out+="; incomplete packet, retry";}}
  return out;
 }
 if(id=="gpio")return String("GPIO ")+c->pins[0]+" = "+digitalRead(c->pins[0]);
 if(id=="i2c"){
  if(!i2cReady)return "Probe external I2C first";String out="I2C addresses:";for(uint8_t a=8;a<120;a++){externalWire.beginTransmission(a);if(externalWire.endTransmission()==0){char b[8];snprintf(b,sizeof b," 0x%02X",a);out+=b;}}return out;
 }
 if(id=="ir"){
  uint32_t pulse=pulseIn(c->pins[0],LOW,150000);return pulse?String("IR LOW pulse: ")+pulse+" us. Raw/NEC transmit available.":"No IR pulse within 150 ms";
 }
 return "No reader";
}
static void irMark(int pin,unsigned us){unsigned long start=micros();while(micros()-start<us){digitalWrite(pin,HIGH);delayMicroseconds(13);digitalWrite(pin,LOW);delayMicroseconds(13);}}
static String transmit(const String &id,const String &data){
 auto *c=find(id);if(!c||c->pins.size()!=c->count)return "Configure pins first";
 if(id=="gpio"){
  if(data!="0"&&data!="1")return "GPIO output accepts exactly 0 or 1";pinMode(c->pins[0],OUTPUT);digitalWrite(c->pins[0],data=="1"?HIGH:LOW);return String("GPIO output set to ")+data;
 }
 if(id=="spi"){
  if(!spiReady)return "Probe SPI first";std::vector<uint8_t> out;if(!V62ModuleCore::hex(data.c_str(),out,128))return "SPI accepts 1-128 HEX bytes";
  radioSPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));digitalWrite(c->pins[3],LOW);for(auto &b:out)b=radioSPI.transfer(b);digitalWrite(c->pins[3],HIGH);radioSPI.endTransaction();return String("SPI RX: ")+bytes(out.data(),out.size());
 }
 if(id=="ir"){
  std::vector<uint8_t> code;if(!V62ModuleCore::hex(data.c_str(),code,4)||code.size()!=4)return "NEC needs four hex bytes: address, inverted address, command, inverted command";
  int pin=c->pins[1];pinMode(pin,OUTPUT);irMark(pin,9000);delayMicroseconds(4500);
  for(auto byte:code)for(int bit=0;bit<8;bit++){irMark(pin,560);delayMicroseconds((byte>>bit)&1?1690:560);}irMark(pin,560);digitalWrite(pin,LOW);return "NEC frame transmitted";
 }
 if(id=="subghz"){
  if(!radioReady)return "Probe CC1101 first";std::vector<uint8_t> packet;if(!V62ModuleCore::hex(data.c_str(),packet,61))return "Packet must contain 1-61 hex bytes";
  strobe(0x36);strobe(0x3b);radioReg(0x3e,0x60);radioReg(0x3f,packet.size());for(auto byte:packet)radioReg(0x3f,byte);strobe(0x35);
  unsigned long start=millis();while(millis()-start<300){uint8_t state=radioReg(0x35,0,true)&0x1f;if(state==1){strobe(0x36);return "Packet TX completed";}delay(1);}strobe(0x36);return "CC1101 TX timeout";
 }
 return "Transmit not implemented for this module";
}
static void stop(){if(radioReady)strobe(0x36);radioSPI.end();externalWire.end();radioReady=false;nfcReady=false;spiReady=false;i2cReady=false;if(lfReady)lfSerial.end();lfReady=false;for(auto &c:configs)for(int p:c.pins)pinMode(p,INPUT);last="All module pins released; configurations retained";}
static String status(){String out="{\"message\":"+q(last)+",\"frequency_hz\":"+String(radioHz)+",\"modules\":[";bool first=true;for(auto &c:configs){if(!first)out+=",";first=false;out+="{\"id\":"+q(c.id)+",\"configured\":"+(c.pins.size()==c.count?"true":"false")+",\"pins\":[";for(size_t i=0;i<c.pins.size();i++){if(i)out+=",";out+=c.pins[i];}out+="]}";}return out+"]}";}
static void routes(V62Server &s){
 s.on("/apps/modules/status",HTTP_GET,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);s.send(200,"application/json",status());});
 s.on("/apps/modules/config",HTTP_POST,[&s](){
  if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String id=s.arg("id"),pins=s.arg("pins"),frequency=s.arg("frequency");auto *c=find(id);long hz=radioHz;uint8_t f[3];
  if(!c||(id=="subghz"&&frequency.length()&&(!V62ModuleCore::integer(frequency.c_str(),hz,300000000,928000000)||!V62ModuleCore::frequency(hz,f)))){s.send(400,"text/plain","Invalid module or frequency");return;}
  // Stop first; GPIO assignments are never applied while another module is running.
  stop();if(pins.isEmpty()){c->pins.clear();Preferences p;p.begin("wdmodules",false);p.remove(id.c_str());p.end();last="Module wiring cleared";s.send(200,"application/json",status());return;}
  if(!validConfig(*c,pins)){s.send(400,"text/plain",last);return;}radioHz=hz;Preferences p;p.begin("wdmodules",false);p.putString(id.c_str(),pins);if(id=="subghz")p.putUInt("frequency",radioHz);p.end();last="Pins saved; use Probe to initialize module";s.send(200,"application/json",status());
 });
 s.on("/apps/modules/probe",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String id=s.arg("id");last=probe(id);s.send(200,"text/plain",last);});
 s.on("/apps/modules/read",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String id=s.arg("id");last=read(id);s.send(200,"text/plain",last);});
 s.on("/apps/modules/transmit",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);String id=s.arg("id"),data=s.arg("data");last=transmit(id,data);s.send(200,"text/plain",last);});
 s.on("/apps/modules/stop",HTTP_POST,[&s](){if(!V61WiFi::authorized(s))return V61WiFi::forbidden(s);stop();s.send(200,"application/json",status());});
}
}
