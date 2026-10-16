idf_component_register(
    SRCS
        "main.cpp"
        "cyberdeck.cpp"
        "${PROJECT_DIR}/managed_components/espressif__arduino-esp32/libraries/NetworkClientSecure/src/ssl_client.cpp"
    INCLUDE_DIRS
        "."
        "../../src"
    REQUIRES
        arduino-esp32
        esp_wifi
        bt
        esp_netif
        lwip
        adafruit_gfx
        adafruit_ili9341
        adafruit_busio
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE V61_IDF_NAT_BUILD=1)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=misleading-indentation)
