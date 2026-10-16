#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <esp_netif.h>
#include <lwip/lwip_napt.h>
#include <esp_log.h>

constexpr int LCD_CS = 10;
constexpr int LCD_DC = 46;
constexpr int LCD_SCK = 12;
constexpr int LCD_MOSI = 11;
constexpr int LCD_MISO = 13;

SPIClass lcdSPI(FSPI);
Adafruit_ILI9341 lcd(&lcdSPI, LCD_DC, LCD_CS, -1);

extern "C" void app_main(void) {
    initArduino();

    void (*volatile nat_api)(u32_t, int) = ip_napt_enable;
    (void)nat_api;

    ESP_LOGI("cyberdeck", "Arduino + display + NAT linked");
    ESP_LOGI("cyberdeck", "Hardware not initialized");
}
