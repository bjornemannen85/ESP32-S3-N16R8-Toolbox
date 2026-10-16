idf_component_register(
    SRCS
        "Adafruit_BusIO_Register.cpp"
        "Adafruit_GenericDevice.cpp"
        "Adafruit_I2CDevice.cpp"
        "Adafruit_SPIDevice.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino-esp32
)
