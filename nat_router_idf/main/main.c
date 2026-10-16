#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/lwip_napt.h"
#include "lwip/ip4_addr.h"
#include "esp_log.h"

/* Isolated build-only prototype. No Wi-Fi credentials, no active NAT by default. */
static const char *TAG = "nat_router_lab";
static esp_netif_t *ap_netif;
static bool nat_enabled = false;

static esp_err_t set_nat(bool enable) {
    if (!ap_netif) return ESP_ERR_INVALID_STATE;
    esp_netif_ip_info_t ip;
    esp_err_t err = esp_netif_get_ip_info(ap_netif, &ip);
    if (err != ESP_OK) return err;
    ip_napt_enable(ip.ip.addr, enable ? 1 : 0);
    nat_enabled = enable;
    ESP_LOGI(TAG, "NAT %s (link-level request; forwarding unverified)", enable ? "requested ON" : "OFF");
    return ESP_OK;
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED && nat_enabled) {
        ESP_LOGW(TAG, "STA disconnected; disabling NAT");
        ESP_ERROR_CHECK(set_nat(false));
    }
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
    esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    wifi_config_t ap = {0};
    memcpy(ap.ap.ssid, "CYBERDECK-NAT-LAB", sizeof("CYBERDECK-NAT-LAB")-1);
    ap.ap.ssid_len = sizeof("CYBERDECK-NAT-LAB")-1;
    ap.ap.channel = 1;
    ap.ap.max_connection = 1;
    /* No password is shipped. Do not start AP on real hardware until secured. */
    ap.ap.authmode = WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    /* Deliberately do not call esp_wifi_start(). Build validates APIs only. */
    ESP_LOGI(TAG, "AP+STA configuration compiled; radio not started; NAT disabled");
    /* Keep set_nat reachable to prove linking, without invoking it. */
    volatile void *symbol_check = (void *)set_nat;
    (void)symbol_check;
}
