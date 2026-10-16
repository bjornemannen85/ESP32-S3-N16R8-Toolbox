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
#include "esp_log.h"

/* Stage 2: compile-only. Explicitly disabled until secure provisioning is implemented. */
#define NAT_LAB_RADIO_ENABLED 0
#define NAT_LAB_AP_SSID "CYBERDECK-NAT-LAB"
#define NAT_LAB_AP_PASSWORD ""  /* No credentials are shipped or committed. */
#define NAT_LAB_STA_SSID ""
#define NAT_LAB_STA_PASSWORD ""

static const char *TAG = "nat_router_stage2";
static esp_netif_t *ap_netif = NULL;
static bool sta_online = false;
static bool nat_enabled = false;
static bool ap_started = false;

static esp_err_t set_nat(bool enable) {
    if (!ap_netif) return ESP_ERR_INVALID_STATE;
    if (enable && (!sta_online || !ap_started)) return ESP_ERR_INVALID_STATE;
    esp_netif_ip_info_t ip;
    esp_err_t err = esp_netif_get_ip_info(ap_netif, &ip);
    if (err != ESP_OK) return err;
    ip_napt_enable(ip.ip.addr, enable ? 1 : 0);
    nat_enabled = enable;
    ESP_LOGI(TAG, "NAPT requested %s; forwarding not hardware-tested", enable ? "ON" : "OFF");
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        sta_online = false;
        if (nat_enabled) ESP_ERROR_CHECK(set_nat(false));
        ESP_LOGW(TAG, "STA disconnected; NAT disabled");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_START) {
        ap_started = true;
        ESP_LOGI(TAG, "AP started; NAT remains disabled until explicitly requested");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STOP) {
        if (nat_enabled) ESP_ERROR_CHECK(set_nat(false));
        ap_started = false;
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        sta_online = true;
        ESP_LOGI(TAG, "STA has IP; NAT remains disabled until explicitly requested");
    }
}

static bool credentials_valid(void) {
    size_t ap_len = strlen(NAT_LAB_AP_PASSWORD);
    size_t sta_len = strlen(NAT_LAB_STA_SSID);
    size_t sta_pw_len = strlen(NAT_LAB_STA_PASSWORD);
    /* Require WPA2 password and a configured upstream. Never start an open AP. */
    return ap_len >= 8 && ap_len <= 63 && sta_len > 0 && sta_len <= 32 && sta_pw_len <= 63;
}

static esp_err_t configure_router(void) {
    if (!credentials_valid()) return ESP_ERR_INVALID_ARG;
    esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL), TAG, "wifi event");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL), TAG, "ip event");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "apsta mode");
    wifi_config_t ap = {0};
    wifi_config_t sta = {0};
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
    /* Compile references to every feature without activating Wi-Fi. */
    volatile void *link_checks[] = {(void *)configure_router, (void *)set_nat};
    (void)link_checks;
    ESP_LOGI(TAG, "Stage 2 built: radio OFF, NAT OFF, no credentials provisioned");
#endif
}
