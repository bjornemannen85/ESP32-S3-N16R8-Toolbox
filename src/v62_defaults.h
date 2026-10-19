#pragma once
namespace V62Defaults {
static constexpr const char HotspotPassword[]="wolfdeck";
// Apply once to existing devices; later user password changes remain intact.
template<class Store> void migrateHotspot(Store &prefs){
 if(!prefs.getBool("wdpass1",false)){
  prefs.putString("ap_pass",HotspotPassword);prefs.putBool("wdpass1",true);
 }
}
}
