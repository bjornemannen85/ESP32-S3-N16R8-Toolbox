#include <Arduino.h>
#include <esp_log.h>
extern void setup();
extern void loop();

extern "C" void app_main(void) {
    initArduino();
    ESP_LOGI("cyberdeck", "Starting full Cyberdeck V6.1");
    setup();

    while (true) {
        loop();
        yield();
    }
}
