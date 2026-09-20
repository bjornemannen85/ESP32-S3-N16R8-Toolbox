#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>
#include <esp_chip_info.h>

WebServer server(80);

static const char *AP_SSID = "ESP32-S3-Toolbox";
static const char *AP_PASS = "toolbox123";

static String htmlEscape(const String &input) {
    String s = input;
    s.replace("&", "&amp;");
    s.replace("<", "&lt;");
    s.replace(">", "&gt;");
    s.replace("\"", "&quot;");
    return s;
}

static String pageStart(const String &title) {
    String h;
    h.reserve(3500);

    h += F(
        "<!DOCTYPE html><html><head>"
        "<meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESP32-S3 Toolbox</title>"
        "<style>"
        "*{box-sizing:border-box}"
        "body{margin:0;background:#101114;color:#eee;font-family:Arial,sans-serif}"
        ".wrap{max-width:850px;margin:auto;padding:18px}"
        "h1{font-size:26px;margin:8px 0 4px}"
        "h2{font-size:21px}"
        ".sub{color:#aaa;margin-bottom:20px}"
        ".card{background:#1b1d22;border:1px solid #30333a;border-radius:14px;"
        "padding:16px;margin:12px 0}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}"
        "a.btn,button{display:block;width:100%;padding:14px;border:0;border-radius:10px;"
        "background:#30343b;color:#fff;text-decoration:none;text-align:center;"
        "font-size:16px;cursor:pointer}"
        "a.btn:hover,button:hover{background:#41464f}"
        "table{width:100%;border-collapse:collapse}"
        "th,td{text-align:left;padding:9px;border-bottom:1px solid #333}"
        "th{color:#aaa}"
        ".good{color:#7ee787}"
        ".warn{color:#ffa657}"
        ".mono{font-family:monospace;word-break:break-all}"
        ".back{margin-top:18px}"
        "</style></head><body><div class='wrap'>"
    );

    h += "<h1>ESP32-S3 TOOLBOX V1</h1>";
    h += "<div class='sub'>" + htmlEscape(title) + "</div>";
    return h;
}

static String pageEnd() {
    return F("</div></body></html>");
}

static void sendPage(const String &html) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", html);
}

static void handleRoot() {
    String h = pageStart("Main menu");

    h += F("<div class='grid'>");
    h += F("<a class='btn' href='/system'>System Info</a>");
    h += F("<a class='btn' href='/wifi'>Wi-Fi Scanner</a>");
    h += F("<a class='btn' href='/ble'>BLE Scanner</a>");
    h += F("<a class='btn' href='/i2c'>I2C Scanner</a>");
    h += F("</div>");

    h += F("<div class='card'>");
    h += "<b>Access Point</b><br>";
    h += "SSID: <span class='mono'>" + String(AP_SSID) + "</span><br>";
    h += "IP: <span class='mono'>" + WiFi.softAPIP().toString() + "</span><br>";
    h += "Free heap: " + String(ESP.getFreeHeap()) + " bytes<br>";
    h += "Free PSRAM: " + String(ESP.getFreePsram()) + " bytes";
    h += F("</div>");

    h += pageEnd();
    sendPage(h);
}

static void handleSystem() {
    esp_chip_info_t info;
    esp_chip_info(&info);

    String h = pageStart("System information");
    h += F("<div class='card'><table>");

    h += "<tr><th>Chip</th><td>ESP32-S3</td></tr>";
    h += "<tr><th>Cores</th><td>" + String(info.cores) + "</td></tr>";
    h += "<tr><th>Revision</th><td>" + String(info.revision) + "</td></tr>";
    h += "<tr><th>CPU</th><td>" + String(ESP.getCpuFreqMHz()) + " MHz</td></tr>";
    h += "<tr><th>Flash</th><td>" + String(ESP.getFlashChipSize()) + " bytes</td></tr>";
    h += "<tr><th>PSRAM</th><td>" + String(ESP.getPsramSize()) + " bytes</td></tr>";
    h += "<tr><th>Free PSRAM</th><td>" + String(ESP.getFreePsram()) + " bytes</td></tr>";
    h += "<tr><th>Heap</th><td>" + String(ESP.getHeapSize()) + " bytes</td></tr>";
    h += "<tr><th>Free heap</th><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
    h += "<tr><th>SDK</th><td>" + String(ESP.getSdkVersion()) + "</td></tr>";
    h += "<tr><th>AP IP</th><td>" + WiFi.softAPIP().toString() + "</td></tr>";

    h += F("</table></div>");
    h += F("<a class='btn back' href='/'>Back</a>");
    h += pageEnd();

    sendPage(h);
}

static String encryptionName(wifi_auth_mode_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}

static void handleWiFi() {
    String h = pageStart("Wi-Fi scanner");
    h += F("<div class='card'>Scanning nearby 2.4 GHz networks...</div>");

    int count = WiFi.scanNetworks(false, true);

    h += "<div class='card'><b>Networks found: " + String(count) + "</b></div>";

    if (count > 0) {
        h += F("<div class='card'><table>"
               "<tr><th>SSID</th><th>RSSI</th><th>CH</th><th>Security</th></tr>");

        for (int i = 0; i < count; i++) {
            h += "<tr>";
            h += "<td>" + htmlEscape(WiFi.SSID(i)) + "</td>";
            h += "<td>" + String(WiFi.RSSI(i)) + " dBm</td>";
            h += "<td>" + String(WiFi.channel(i)) + "</td>";
            h += "<td>" + encryptionName(WiFi.encryptionType(i)) + "</td>";
            h += "</tr>";
        }

        h += F("</table></div>");
    } else {
        h += F("<div class='card warn'>No networks found.</div>");
    }

    WiFi.scanDelete();

    h += F("<a class='btn' href='/wifi'>Rescan</a>");
    h += F("<a class='btn back' href='/'>Back</a>");
    h += pageEnd();

    sendPage(h);
}

static void handleBLE() {
    String h = pageStart("Bluetooth LE scanner");
    h += F("<div class='card'>Scanning BLE advertisements for 5 seconds...</div>");

    BLEScan *scanner = BLEDevice::getScan();
    scanner->setActiveScan(true);
    scanner->setInterval(100);
    scanner->setWindow(99);

    BLEScanResults results = scanner->start(5, false);
    int count = results.getCount();

    h += "<div class='card'><b>BLE devices found: " + String(count) + "</b></div>";

    if (count > 0) {
        h += F("<div class='card'><table>"
               "<tr><th>Name</th><th>Address</th><th>RSSI</th></tr>");

        for (int i = 0; i < count; i++) {
            BLEAdvertisedDevice dev = results.getDevice(i);

            String name = dev.haveName()
                ? String(dev.getName().c_str())
                : String("(unnamed)");

            String address = String(dev.getAddress().toString().c_str());

            h += "<tr>";
            h += "<td>" + htmlEscape(name) + "</td>";
            h += "<td class='mono'>" + htmlEscape(address) + "</td>";
            h += "<td>" + String(dev.getRSSI()) + " dBm</td>";
            h += "</tr>";
        }

        h += F("</table></div>");
    } else {
        h += F("<div class='card warn'>No BLE devices found.</div>");
    }

    scanner->clearResults();

    h += F("<a class='btn' href='/ble'>Rescan</a>");
    h += F("<a class='btn back' href='/'>Back</a>");
    h += pageEnd();

    sendPage(h);
}

static void handleI2C() {
    String h = pageStart("I2C scanner");

    h += F(
        "<div class='card'>"
        "Default V1 pins:<br>"
        "<b>SDA = GPIO 8</b><br>"
        "<b>SCL = GPIO 9</b><br><br>"
        "Use 3.3 V devices unless the module is explicitly ESP32-safe."
        "</div>"
    );

    int found = 0;

    h += F("<div class='card'><table>"
           "<tr><th>Address</th><th>Status</th></tr>");

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            char hexAddr[8];
            snprintf(hexAddr, sizeof(hexAddr), "0x%02X", address);

            h += "<tr><td class='mono'>";
            h += hexAddr;
            h += "</td><td class='good'>FOUND</td></tr>";
            found++;
        }
    }

    h += F("</table></div>");

    if (found == 0) {
        h += F("<div class='card warn'>No I2C devices detected.</div>");
    } else {
        h += "<div class='card good'>Devices found: " + String(found) + "</div>";
    }

    h += F("<a class='btn' href='/i2c'>Rescan</a>");
    h += F("<a class='btn back' href='/'>Back</a>");
    h += pageEnd();

    sendPage(h);
}

static void handleNotFound() {
    server.send(404, "text/plain", "404 - Not found");
}

void setup() {
    Serial.begin(115200);
    delay(1200);

    Serial.println();
    Serial.println("================================");
    Serial.println(" ESP32-S3 N16R8 TOOLBOX V1");
    Serial.println("================================");
    Serial.printf("Flash: %u bytes\n", ESP.getFlashChipSize());

    if (psramFound()) {
        Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("WARNING: PSRAM NOT FOUND");
    }

    Wire.begin(8, 9);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);

    BLEDevice::init("ESP32-S3-Toolbox");

    server.on("/", handleRoot);
    server.on("/system", handleSystem);
    server.on("/wifi", handleWiFi);
    server.on("/ble", handleBLE);
    server.on("/i2c", handleI2C);
    server.onNotFound(handleNotFound);

    server.begin();

    Serial.println();
    Serial.println("Toolbox ready");
    Serial.printf("SSID: %s\n", AP_SSID);
    Serial.printf("Password: %s\n", AP_PASS);
    Serial.print("Open: http://");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    server.handleClient();
    delay(2);
}
