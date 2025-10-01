/*
   ESP32 Google Geolocation (WiFi scan version, fixed)
   - Scans nearby WiFi APs
   - Builds correct JSON payload
   - Sends to Google Geolocation API
   - Prints Lat, Lng, Accuracy
   - Needs ArduinoJson library (6.x)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ----------- USER CONFIG -------------
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";
const char* GOOGLE_API_KEY = "";  // replace with your API key
// -------------------------------------

const char* GEOLOCATE_ENDPOINT = "https://www.googleapis.com/geolocation/v1/geolocate?key=";

// ===== Helper to format BSSID correctly =====
String formatBSSID(const uint8_t *bssid) {
  char buf[18];
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
          bssid[0], bssid[1], bssid[2],
          bssid[3], bssid[4], bssid[5]);
  return String(buf);
}

#define BUTTON_PIN 23

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\nESP32 -> Google Geolocation");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);   // clear prev state
  delay(100);

  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi not connected, but proceeding with scan.");
  }

}

void loop() {

if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed! Performing extraction...");
    performGeolocationWithScan();

    // debounce / prevent multiple triggers
    delay(2000);
  }
  // small idle delay
  delay(50);
}

void performGeolocationWithScan() {
  Serial.println("\nPerforming WiFi scan...");
  int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    Serial.println("No networks found.");
    return;
  }
  Serial.printf("Found %d networks\n", n);

  // Build JSON
  size_t capacity = JSON_ARRAY_SIZE(n) + n * JSON_OBJECT_SIZE(4) + 512;
  DynamicJsonDocument doc(capacity);

  doc["considerIp"] = true;
  JsonArray wifiArr = doc.createNestedArray("wifiAccessPoints");

  for (int i = 0; i < n; i++) {
    const uint8_t* bssid = WiFi.BSSID(i);
    if (!bssid) continue;   // skip invalid

    String mac = formatBSSID(bssid);

    JsonObject ap = wifiArr.createNestedObject();
    ap["macAddress"] = mac;                // ✅ only proper MAC
    ap["signalStrength"] = WiFi.RSSI(i);
    ap["channel"] = WiFi.channel(i);

    Serial.printf("%d: %s  RSSI:%d  CH:%d  SSID:%s\n",
                  i, mac.c_str(), WiFi.RSSI(i), WiFi.channel(i), WiFi.SSID(i).c_str());
  }

  // Serialize payload
  String payload;
  serializeJson(doc, payload);
  Serial.println("\nRequest payload:");
  Serial.println(payload);

  // Send to Google API
  HTTPClient http;
  String url = String(GEOLOCATE_ENDPOINT) + GOOGLE_API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  Serial.println("\nPOSTing to Google Geolocation API...");
  int httpCode = http.POST((uint8_t*)payload.c_str(), payload.length());

  if (httpCode > 0) {
    Serial.printf("HTTP response code: %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);

    if (httpCode == 200) {
      DynamicJsonDocument respDoc(1024);
      if (deserializeJson(respDoc, response) == DeserializationError::Ok) {
        double lat = respDoc["location"]["lat"];
        double lng = respDoc["location"]["lng"];
        double accuracy = respDoc["accuracy"];

        Serial.println("\n=== Geolocation Result ===");
        Serial.printf("Latitude : %.7f\n", lat);
        Serial.printf("Longitude: %.7f\n", lng);
        Serial.printf("Accuracy (m): %.2f\n", accuracy);
        Serial.println("==========================");
      }
    }
  } else {
    Serial.printf("POST failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
