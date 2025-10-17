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
#include <DNSServer.h>
#include <map>
#include "crypto.h"

// BEWARE: Important! Change WiFi password here!
uint8_t ssid[33] = "Mercedes-Benz";
uint8_t password[33] = "12345678";
String hash;
String seed;
String salt;

uint8_t update = 1;
bool rebootPending = false;
unsigned long rebootTime = 0;
unsigned long ota_progress_millis = 0;

DNSServer dnsServer;
AsyncWebServer server(80);

std::map<IPAddress, bool> authorizedClients;

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


// redirects all unmatched AsyncWebServer requests
class CaptiveRequestHandler : public AsyncWebHandler {
public:
  bool canHandle(AsyncWebServerRequest *request) { return true; }
  void handleRequest(AsyncWebServerRequest *request) {
    request->redirect("/");
  }
};


// read IP from client list
bool isAuthorized(AsyncWebServerRequest *req) {
  IPAddress ip = req->client()->remoteIP();
  auto it = authorizedClients.find(ip);
  return (it != authorizedClients.end() && it->second);
}


// write IP to client list
void setAuthorized(AsyncWebServerRequest *req, bool state) {
  IPAddress ip = req->client()->remoteIP();
  authorizedClients[ip] = state;
}


// schedule reboot
void scheduleReboot(unsigned long delayMs) {
  rebootPending = true;
  rebootTime = millis() + delayMs;
  Serial.println("rebooting...");
}


// perform reboot
void handleReboot() {
  if (rebootPending && (long)(millis() - rebootTime) > 0) {
    rebootPending = false;
    dnsServer.stop();
    server.end();
    authorizedClients.clear();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(200);
    ESP.restart();
  }
}


// MIME content type based on file name suffix
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".pem")) return "application/x-x509-ca-cert";
  if (filename.endsWith(".crt")) return "application/x-x509-ca-cert";
  if (filename.endsWith(".key")) return "application/octet-stream";
  if (filename.endsWith(".webm")) return "video/webm";
  if (filename.endsWith(".mp4")) return "video/mp4";
  return "text/plain";
}

// write data -> into configJson bidirectional transport JSON string processed by HTML client
void updateJson() {
  DynamicJsonDocument doc(1024);
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
  calibration["duty"]     = duty;
  JsonObject modeObj = doc.containsKey(mode) ? doc[mode].as<JsonObject>() : doc.createNestedObject(mode);
  modeObj["offset_nv"] = offset_nv;
  modeObj["offset_nh"] = offset_nh;
  doc["current_mode"] = mode;
  JsonObject level = doc.containsKey("level") ? doc["level"].as<JsonObject>() : doc.createNestedObject("level");
  level["fzgn_vl"] = FS_340h.FZGN_VL;
  level["fzgn_vr"] = FS_340h.FZGN_VR;
  level["fzgn_hl"] = FS_340h.FZGN_HL;
  level["fzgn_hr"] = FS_340h.FZGN_HR;
  JsonObject w = doc.containsKey("wifi") ? doc["wifi"].as<JsonObject>() : doc.createNestedObject("wifi");
  w["hash"].set(hash);
  w["seed"].set(seed);
  w["salt"].set(salt);
  serializeJson(doc, configJson);
}

// read data <- from configJson bidirectional transport JSON string processed by HTML client
void readJson() {
  DynamicJsonDocument doc(1024);
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
  if (doc.containsKey("wifi")) {
    JsonObject w = doc["wifi"];
    if (w.containsKey("hash")) hash = w["hash"].as<String>();
    if (w.containsKey("seed")) seed = w["seed"].as<String>();
    if (w.containsKey("salt")) salt = w["salt"].as<String>();
  }
  limitOffset(&offset_nv);
  limitOffset(&offset_nh);
}

// handle HTTP_POST actions
void webConfig() {
  // initial loading of configJson bidirectional transport JSON string
  if (update == 1) {
    getCalibration();
    updateJson();
    update = 0;
  }
  // save PWM Voltage Calibration to LittleFS
  else if (update == 2) {
    readJson();
    updateCalibration("calib_vl", calib_vl);
    updateCalibration("calib_vr", calib_vr);
    updateCalibration("calib_hl", calib_hl);
    updateCalibration("calib_hr", calib_hr);
    update = 0;
  }
  // receive axle level custom offset from HTML
  else if (update == 3) {
    readJson();
    update = 0;
  }
  // save axle level custom offset to LittleFS
  else if (update == 4) {
    readJson();
    updateSettings(mode, "offset_nv", offset_nv);
    updateSettings(mode, "offset_nh", offset_nh);
    update = 0;
  }
  // send wifi credentials
  else if (update == 5) {
    readJson();
    cryptUpdateWifi();
    updateJson();
    update = 0;
  }
  // receive wifi credentials
  else if (update == 6) {
    readJson();
    if(cryptGetWifi()) {
      updateJson();
      updateWifi(hash, seed);
      scheduleReboot(5000);
    }
    update = 0;
  }
}

void wifiSetup() {
  WiFi.persistent(false);

  // read wifi credentials from LittleFS
  getWifi(hash, seed);
  if (hash.length() < 32 || seed.length() < 32 || getMac(hw_key) != ESP_OK) {
    Serial.println("No WiFi config found, starting default AP...");
  } else {
    memset(ssid, 0x00, sizeof(ssid));
    memset(password, 0x00, sizeof(password));
    hexToBytes(hash, enc_ssid);
    hexToBytes(seed, enc_pass);
    aes_decrypt(enc_ssid, hw_key, ssid, sizeof(enc_ssid), sizeof(hw_key));
    aes_decrypt(enc_pass, hw_key, password, sizeof(enc_pass), sizeof(hw_key));
    ssid[32] = '\0';
    password[32] = '\0';
  }

  // wifi up
  WiFi.mode(WIFI_AP);
  WiFi.softAP((char *)ssid, (char *)password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Access Point started");

  // fake DNS server
  dnsServer.start(53, "*", WiFi.softAPIP());

  // redirect captive portal detection endpoints
  // *** wikipedia.org/wiki/Captive_portal ***
  {
    // Android / ChromeOS
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(204, "text/plain", "");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(204, "text/plain", "");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });

    // Apple
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/html", "<html><head><title>Success</title></head><body>Success</body></html>");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/html", "<html><head><title>Success</title></head><body>Success</body></html>");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });

    // Windows
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/plain", "Microsoft Connect Test");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/plain", "Microsoft NCSI");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });

    // Linux (GNOME / KDE / Firefox)
    server.on("/check_network_status.txt", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/plain", "NetworkManager is online");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });
    server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest *req){
      if (isAuthorized(req)) {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=https://support.mozilla.org/kb/captive-portal'/>");
      } else {
        req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
      }
    });
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
      // KDE probe comes to "/" with Host == "networkcheck.kde.org"
      if (req->host() == String("networkcheck.kde.org")) {
        if (isAuthorized(req)) {
          req->send(200, "text/plain", "OK");
        } else {
          req->send(200, "text/html", "<meta http-equiv='refresh' content='0;url=/' />");
        }
        return;
      }
      // normal index for other hosts/clients
      req->send(LittleFS, "/index.html", "text/html");
    });
  }

  // captive check succeeded
  server.on("/authorized", HTTP_GET, [](AsyncWebServerRequest *req){
    setAuthorized(req, true);
    req->send(204);
  });

  // wifi settings
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(LittleFS, "/settings.html", "text/html");
  });

  // generate RSA 2048-bit private.pem + public.pem key pair files
  server.on(pubKeyFile, HTTP_GET, [](AsyncWebServerRequest *request) {
    generateKeys();
    request->send(LittleFS, pubKeyFile, "application/x-pem-file");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/config.html", "text/html");
  });

  // receive instructions (update flag)
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

  // bidirectional transport JSON string processed by HTML client
  server.on(config, HTTP_GET, [](AsyncWebServerRequest *request){
    updateJson();
    request->send(200, "application/json", configJson);
  });

  // captive portal
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

  // file server
  server.onNotFound([](AsyncWebServerRequest* request) {
    String path = request->url();
    if (LittleFS.exists(path)) {
      String contentType = getContentType(path);
      AsyncWebServerResponse* response = request->beginResponse(LittleFS, path, contentType);
      if (path.endsWith(".png") || path.endsWith(".jpg") || path.endsWith(".js") || path.endsWith(".webm") || path.endsWith(".mp4")) {
        response->addHeader("Cache-Control", "public, max-age=31536000");
      }
      request->send(response);
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
        wifiConnected = true;
        wifi_up = time;
        awake(100);
      } else {
        wifiConnected = false;
      }
      if (wifi && (time - wifi_up > wifi_down )) {
        wifi = false;
        wifiConnected = false;
        Serial.println("HTTP server stop");
        dnsServer.stop();            // stop DNS redirection
        server.end();                // stop HTTP server
        authorizedClients.clear();   // reset client list
        WiFi.softAPdisconnect(true); // disconnect AP and clients
        WiFi.mode(WIFI_OFF);         // turn off WiFi hardware
        vTaskDelete(NULL);
      }
    }
    if (wifi) {
      ElegantOTA.loop();
      webConfig();
      handleReboot();
      dnsServer.processNextRequest();
    }
    delay(100);
  }
}
