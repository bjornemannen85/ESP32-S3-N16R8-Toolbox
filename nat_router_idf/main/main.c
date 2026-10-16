#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/lwip_napt.h"
#include "lwip/ip4_addr.h"
#include "esp_log.h"

/* Build-only stage: never start radios until secure provisioning and integration. */
#define NAT_LAB_RADIO_ENABLED 0
#define NAT_LAB_AP_SSID "CYBERDECK-NAT-LAB"
#define NAT_LAB_AP_PASSWORD ""
#define NAT_LAB_STA_SSID ""
#define NAT_LAB_STA_PASSWORD ""

static const char *TAG = "nat_router_stage3";
static esp_netif_t *ap_netif;
static esp_netif_t *sta_netif;
static bool sta_online;
static bool nat_enabled;
static bool ap_started;
static bool nat_requested;

static esp_err_t set_nat(bool enable) {
    if (!ap_netif) return ESP_ERR_INVALID_STATE;
    if (enable && (!sta_online || !ap_started)) return ESP_ERR_INVALID_STATE;
    esp_netif_ip_info_t ip;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(ap_netif, &ip), TAG, "AP IP");
    ip_napt_enable(ip.ip.addr, enable ? 1 : 0);
    nat_enabled = enable;
    ESP_LOGI(TAG, "NAPT %s (packet forwarding not tested)", enable ? "enabled" : "disabled");
    return ESP_OK;
}

static esp_err_t update_ap_dns(void) {
    if (!sta_netif || !ap_netif || !sta_online) return ESP_ERR_INVALID_STATE;
    esp_netif_dns_info_t dns;
    ESP_RETURN_ON_ERROR(esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns), TAG, "STA DNS");
    if (dns.ip.type != ESP_IPADDR_TYPE_V4 || dns.ip.u_addr.ip4.addr == 0) return ESP_ERR_INVALID_STATE;
    ESP_RETURN_ON_ERROR(esp_netif_dhcps_stop(ap_netif), TAG, "stop AP DHCP");
    esp_err_t err = esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
    esp_err_t restart = esp_netif_dhcps_start(ap_netif);
    if (err != ESP_OK) return err;
    if (restart != ESP_OK) return restart;
    ESP_LOGI(TAG, "AP DHCP DNS updated from upstream");
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        sta_online = false;
        if (nat_enabled) (void)set_nat(false);
        ESP_LOGW(TAG, "STA disconnected: NAT off; no automatic NAT re-enable");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_START) {
        ap_started = true;
        ESP_LOGI(TAG, "AP started, NAT off by default");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STOP) {
        if (nat_enabled) (void)set_nat(false);
        ap_started = false;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        sta_online = true;
        esp_err_t err = update_ap_dns();
        if (err != ESP_OK) ESP_LOGW(TAG, "DHCP DNS update failed: %s", esp_err_to_name(err));
        ESP_LOGI(TAG, "STA online; NAT %s", nat_requested ? "requested but not auto-enabled" : "not requested");
    }
}

static bool credentials_valid(void) {
    size_t a = strlen(NAT_LAB_AP_PASSWORD), s = strlen(NAT_LAB_STA_SSID), p = strlen(NAT_LAB_STA_PASSWORD);
    return a >= 8 && a <= 63 && s > 0 && s <= 32 && p <= 63;
}

static esp_err_t configure_router(void) {
    if (!credentials_valid()) return ESP_ERR_INVALID_ARG;
    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();
    if (!sta_netif || !ap_netif) return ESP_ERR_NO_MEM;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL), TAG, "wifi event");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL), TAG, "IP event");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "APSTA mode");
    wifi_config_t ap = {0}, sta = {0};
    memcpy(ap.ap.ssid, NAT_LAB_AP_SSID, strlen(NAT_LAB_AP_SSID));
    ap.ap.ssid_len = strlen(NAT_LAB_AP_SSID);
    memcpy(ap.ap.password, NAT_LAB_AP_PASSWORD, strlen(NAT_LAB_AP_PASSWORD));
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap.ap.max_connection = 4;
    memcpy(sta.sta.ssid, NAT_LAB_STA_SSID, strlen(NAT_LAB_STA_SSID));
    memcpy(sta.sta.password, NAT_LAB_STA_PASSWORD, strlen(NAT_LAB_STA_PASSWORD));
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap), TAG, "AP config");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta), TAG, "STA config");
    return esp_wifi_start();
}

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
#if NAT_LAB_RADIO_ENABLED
    ESP_ERROR_CHECK(configure_router());
#else
    volatile void *checks[] = {(void *)configure_router, (void *)set_nat, (void *)update_ap_dns};
    (void)checks;
    ESP_LOGI(TAG, "Stage 3 build-only: Wi-Fi OFF, NAT OFF; DHCP/DNS logic compiled");
#endif
}
