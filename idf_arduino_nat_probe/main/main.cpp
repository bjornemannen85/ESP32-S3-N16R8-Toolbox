#include <Arduino.h>
#include <esp_log.h>
extern void setup();
extern void loop();

extern "C" void app_main(void) {
    initArduino();
    ESP_LOGI("cyberdeck", "Starting Cyberdeck V6.2 Full Access");
    setup();

    while (true) {
        loop();
        yield();
    }
}
