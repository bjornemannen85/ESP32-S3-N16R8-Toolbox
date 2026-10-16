idf_component_register(
    SRCS
        "main.cpp"
        "cyberdeck.cpp"
    INCLUDE_DIRS
        "."
        "../../src"
    REQUIRES
        arduino-esp32
        esp_wifi
        esp_netif
        lwip
        adafruit_gfx
        adafruit_ili9341
        adafruit_busio
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE V61_IDF_NAT_BUILD=1)
