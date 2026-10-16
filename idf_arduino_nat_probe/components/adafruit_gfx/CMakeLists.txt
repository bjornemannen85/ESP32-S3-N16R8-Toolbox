idf_component_register(
    SRCS
        "Adafruit_GFX.cpp"
        "Adafruit_GrayOLED.cpp"
        "Adafruit_SPITFT.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino-esp32 adafruit_busio
)
