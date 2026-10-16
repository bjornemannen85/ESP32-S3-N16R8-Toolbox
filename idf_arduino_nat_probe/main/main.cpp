#include <Arduino.h>
#include <WiFi.h>
#include <esp_netif.h>
#include <lwip/lwip_napt.h>
#include <esp_log.h>

// Compile/link probe only: no radio, AP, credentials or NAT enabled.
extern "C" void app_main(void) {
    initArduino();
    void (*volatile nat_api)(u32_t, int) = ip_napt_enable;
    (void)nat_api;
    ESP_LOGI("v61_probe", "Arduino+ESP-IDF NAT symbols linked; Wi-Fi OFF");
}
