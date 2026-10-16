idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_wifi
        esp_netif
        lwip
        arduino-esp32
        adafruit_gfx
        adafruit_ili9341
        adafruit_busio
)
