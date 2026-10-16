#include <Arduino.h>
#include <WiFi.h>
#include <esp_netif.h>
#include <lwip/lwip_napt.h>
#include <esp_log.h>

static const char *TAG = "cyberdeck_nat";

extern "C" void app_main(void) {
    initArduino();

    // Verify that Arduino and ESP-IDF NAT are linked.
    void (*volatile nat_api)(u32_t, int) = ip_napt_enable;
    (void)nat_api;

    ESP_LOGI(TAG, "Cyberdeck V6.1 NAT integration");
    ESP_LOGI(TAG, "Arduino initialized");
    ESP_LOGI(TAG, "NAT API linked");
    ESP_LOGI(TAG, "Radio disabled during integration test");
}
