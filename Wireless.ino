/*
  -----------------------
  ElegantOTA - Async
  -----------------------

  NOTE: Make sure you have enabled Async Mode in ElegantOTA before compiling!
  Guide: https://docs.elegantota.pro/async-mode/

  This example provides with a bare minimal app with ElegantOTA functionality which works
  with AsyncWebServer.

  Github: https://github.com/ayushsharma82/ElegantOTA
  WiKi: https://docs.elegantota.pro

  -------------------------------

  Upgrade to ElegantOTA Pro: https://elegantota.pro
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// BEWARE: Important! Change WiFi password here!
const char* ssid = "ESP32";
const char* password = "12345678";

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void wifiSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Access Point started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA Async. visit http://192.168.4.1/update");
  });

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Hi! This is ElegantOTA Async. visit http://192.168.4.1/update");
}

void wifiEvent(void *parameter) {
  const unsigned long wifi_down = 300000; // Stop WiFi after 5 minutes
  static unsigned long wifi_up = 0;
  static unsigned long time = 0;
  static bool wifi = true;
  while (true) {
    if (millis() - time > 5000) {
      time = millis();
      if (WiFi.softAPgetStationNum() > 0) {
        wifi_up = time;
      }
      if (wifi && (time - wifi_up > wifi_down )) {
        wifi = false;
        Serial.println("HTTP server stop");
        server.end();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        vTaskDelete(NULL);
      }
    }
    if (wifi) {
      ElegantOTA.loop();
    }
    delay(10);
  }
}
