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
const char* ssid = "Mercedes-Benz";
const char* password = "12345678";

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

uint8_t update = 1;

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

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void updateJson() {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, configJson);
  if (err) {
    Serial.print("Cannot deserialize the current JSON object: ");
    Serial.println(err.c_str());
    doc.to<JsonObject>();
  }
  JsonObject calibration = doc.containsKey("calibration") ? doc["calibration"].as<JsonObject>() : doc.createNestedObject("calibration");
  calibration["calib_vl"] = calib_vl;
  calibration["calib_vr"] = calib_vr;
  calibration["calib_hl"] = calib_hl;
  calibration["calib_hr"] = calib_hr;
  JsonObject modeObj = doc.containsKey(mode) ? doc[mode].as<JsonObject>() : doc.createNestedObject(mode);
  modeObj["offset_nv"] = offset_nv;
  modeObj["offset_nh"] = offset_nh;
  doc["current_mode"] = mode;
  serializeJson(doc, configJson);
}

void readJson() {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, configJson);
  if (err) {
    Serial.print("Cannot deserialize the current JSON object: ");
    Serial.println(err.c_str());
    return;
  }
  if (doc.containsKey("calibration")) {
    JsonObject c = doc["calibration"];
    if (c.containsKey("calib_vl")) calib_vl = c["calib_vl"];
    if (c.containsKey("calib_vr")) calib_vr = c["calib_vr"];
    if (c.containsKey("calib_hl")) calib_hl = c["calib_hl"];
    if (c.containsKey("calib_hr")) calib_hr = c["calib_hr"];
  }
  if (doc.containsKey(mode)) {
    JsonObject m = doc[mode];
    if (m.containsKey("offset_nv")) offset_nv = m["offset_nv"];
    if (m.containsKey("offset_nh")) offset_nh = m["offset_nh"];
  }
  limitOffset(&offset_nv);
  limitOffset(&offset_nh);
}

void webConfig() {
  if (update == 1) {
    getCalibration();
    updateJson();
    update = 0;
  } 
  else if (update == 2) {
    readJson();
    updateCalibration("calib_vl", calib_vl);
    updateCalibration("calib_vr", calib_vr);
    updateCalibration("calib_hl", calib_hl);
    updateCalibration("calib_hr", calib_hr);
    update = 0;
  }
  else if (update == 3) {
    readJson();
    update = 0;
  }
  else if (update == 4) {
    readJson();
    updateSettings(mode, "offset_nv", offset_nv);
    updateSettings(mode, "offset_nh", offset_nh);
    update = 0;
  }
}

void wifiSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Access Point started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", 
      "Hi! This is ElegantOTA. visit: http://192.168.4.1/update\n"
      "AIRmatic Settings: http://192.168.4.1/config");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/config.html", "text/html");
  });

  server.on(
    "/config",
    HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (request->hasParam("update", false)) {
        String updateParam = request->getParam("update", false)->value();
        update = updateParam.toInt();
      }
      request->send(200, "text/plain", "OK");
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (len > 0) {
        configJson = String((char*)data, len);
      }
      if (request->hasParam("update", false)) {
        String updateParam = request->getParam("update", false)->value();
        update = updateParam.toInt();
      }
    }
  );

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
    updateJson();
    request->send(200, "application/json", configJson);
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    String path = request->url();
    if (LittleFS.exists(path)) {
      String contentType = getContentType(path);
      request->send(LittleFS, path, contentType);
    } else {
      request->send(404, "text/plain", "404: File not found");
    }
  });

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Hi! This is ElegantOTA. visit: http://192.168.4.1/update");
  Serial.println("AIRmatic Settings: http://192.168.4.1/config");
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
      webConfig();
    }
    delay(100);
  }
}
