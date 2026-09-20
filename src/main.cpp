#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("ESP32-S3 N16R8 TOOLBOX");
    Serial.println("Boot OK");

    if (psramFound()) {
        Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("PSRAM: NOT FOUND");
    }

    Serial.printf("Flash: %u bytes\n", ESP.getFlashChipSize());
}

void loop() {
    delay(1000);
}
