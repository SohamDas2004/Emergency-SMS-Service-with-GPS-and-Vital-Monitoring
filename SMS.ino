#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "mbedtls/base64.h" 

#define BUTTON_PIN 23

const char* ssid = "TECNO SPARK 10C";
const char* password = "abhra2004";

// Twilio credentials
const char* account_sid = "";
const char* auth_token = "";
const char* from_number = "";
const char* to_number = "";


// GPS coordinates
double LAT =30.5215;
double LON =90.7196;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}


void loop() { 
  //if (digitalRead(BUTTON_PIN) == LOW) { 
    delay(10000);
    Serial.println("Button pressed! Performing extraction..."); 
    sendsms(); // debounce / prevent multiple triggers 
    // small idle delay 
    delay(10000); 
}


void sendsms() {

  // Build message
  String msg = "GPS link: https://www.google.com/maps?q=" + String(LAT, 6) + "," + String(LON, 6);

  // Encode credentials in Base64
  String credentials = String(account_sid) + ":" + String(auth_token);
  String encodedCredentials = base64Encode(credentials);
  delay(1000);
  WiFiClientSecure client;
  client.setInsecure();  // <-- ignore SSL verification
  delay(1000);
  Serial.println("Connecting to api.twilio.com...");

  if (!client.connect("api.twilio.com", 443)) {
    Serial.println("Connection failed!");
    return;
  }

  String url = "/2010-04-01/Accounts/" + String(account_sid) + "/Messages.json";

  String postData = "To=" + String(to_number) + "&From=" + String(from_number) + "&Body=" + msg;

  // Send HTTPS POST request
  client.println("POST " + url + " HTTP/1.1");
  delay(1000);
  client.println("Host: api.twilio.com");
  client.println("Authorization: Basic " + encodedCredentials);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.println(postData);
  delay(1000);

  // Read response
  while (client.connected()) {
    delay(1000);
    String line = client.readStringUntil('\n');
    delay(1000);
    Serial.println(line);
    if (line == "\r") break; // headers done
  }
}

String base64Encode(const String& input) {
  size_t out_len;
  unsigned char output[512]; // should be enough for small strings
  mbedtls_base64_encode(output, sizeof(output), &out_len, 
                        (const unsigned char*)input.c_str(), input.length());
  return String((char*)output);
}

