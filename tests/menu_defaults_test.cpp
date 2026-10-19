#include "v62_categories.h"
#include "v62_defaults.h"
#include "WebServer.h"
#include "v62_parameters.h"
#include <map>
#include <string>
#include <cassert>
#include <iostream>
struct Store {
 bool migrated=false;int writes=0;
 std::map<std::string,std::string> values;
 bool getBool(const char*,bool){return migrated;}
 void putBool(const char*,bool value){migrated=value;++writes;}
 void putString(const char *name,const char *value){values[name]=value;++writes;}
};
int main(){
 using namespace V62Menu;
 assert(category("/wifi/ap")==Settings);assert(category("/wifi/status")==Network);
 assert(category("/api/v61/ble-transfer/start")==Bluetooth);
 assert(category("/api/v61/uart/start")==Hardware);assert(category("/api/v61/adc/read")==Hardware);
 assert(category("/scripts/backup")==Files);assert(category("/api/v61/sd/status")==Files);
 assert(category("/toolbox/memory")==System);assert(category("/api/v61/scan/history")==Network);
 for(const auto &p:v62Parameters)assert(category(p.path)>=0&&category(p.path)<Count);
 for(int i=0;i<8;i++){
  int x=HomeX+(i%2)*HomeDX,y=HomeY+(i/2)*HomeDY;
  assert(homeIndex(x,y)==i);assert(homeIndex(x+HomeW-1,y+HomeH-1)==i);
  assert(homeIndex(x+HomeW,y)==-1);assert(homeIndex(x,y+HomeH)==-1);
 }
 assert(homeIndex(0,0)==-1);assert(homeIndex(320,240)==-1);
 Store prefs;prefs.values["ap_pass"]="legacy-password";prefs.values["admin"]="private-token";prefs.values["n0p"]="router-password";
 V62Defaults::migrateHotspot(prefs);assert(prefs.values["ap_pass"]=="wolfdeck"&&prefs.migrated&&prefs.writes==2);
 assert(prefs.values["admin"]=="private-token"&&prefs.values["n0p"]=="router-password");
 prefs.values["ap_pass"]="my-later-password";
 V62Defaults::migrateHotspot(prefs);assert(prefs.values["ap_pass"]=="my-later-password"&&prefs.writes==2);
 Store fresh;V62Defaults::migrateHotspot(fresh);assert(fresh.values["ap_pass"]=="wolfdeck");
 std::cout<<"PASS: shared categories, every route classified, home touch geometry/gaps, one-time hotspot migration, custom/admin/upstream credentials preserved\n";
}
