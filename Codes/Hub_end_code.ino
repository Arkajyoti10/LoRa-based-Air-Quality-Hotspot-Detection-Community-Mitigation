#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
// --- 1. WI-FI CREDENTIALS ---
#define WIFI_SSID "ABCD"          
#define WIFI_PASSWORD "123456789" 


const char* cloudAPIEndpoint = "https://script.google.com/macros/s/AKfycbw2xkgP5GVBTsl2anjjQawLH0IocxaDcs1Z9S_kAz0-A9Vu8GuO4c0cR6YnDJ1Chdf1/exec"; 

// --- 3. LoRa Pin Definitions (Zero PCB Match) ---
#define SCK     18
#define MISO    19
#define MOSI    23
#define SS      5
#define RST     14
#define DIO0    26

// LoRa Data Structure (Node 1 se 100% Match)
struct _attribute((packed_)) DataPacket {
  float temperature;
  float humidity;
  int gasValue;
  int dustValue;      
  bool sprayStatus;   
};

DataPacket incomingPacket;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("🛰️ S NODE: CLOUD INTERFACE LAYER ACTIVE");
  Serial.println("==================================================");

  // Wi-Fi Connection Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi Connected Successfully!");

  // LoRa Radio Hardware Initialization
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(866E6)) { 
    Serial.println("❌ S Node: LoRa hardware nahi mila!");
    while (1);
  }
  Serial.println("🛰️ LoRa Receiver Connected at 866MHz!");
  Serial.println("==================================================\n");
}

void loop() {
  // Check if a radio packet has arrived from Node 1
  int packetSize = LoRa.parsePacket();
  
  if (packetSize == sizeof(incomingPacket)) {
    // Read packet into struct
    LoRa.readBytes((uint8_t*)&incomingPacket, sizeof(incomingPacket));
    
    Serial.println("\n📥 [Node 1 Data Received] -> Sending to Cloud API...");

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(cloudAPIEndpoint);
      http.addHeader("Content-Type", "application/json");
      
     
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

     
      String jsonPayload = "{\"temperature\":" + String(incomingPacket.temperature) + 
                           ",\"humidity\":" + String(incomingPacket.humidity) + 
                           ",\"gasValue\":" + String(incomingPacket.gasValue) + 
                           ",\"dustValue\":" + String(incomingPacket.dustValue) + 
                           ",\"lastState\":" + String(incomingPacket.sprayStatus ? 1 : 0) + "}";

      
      int httpCode = http.POST(jsonPayload);
      
      if (httpCode > 0) {
        String response = http.getString();
        Serial.print("☁️ Cloud Engine Response: "); Serial.println(response);

        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
          const char* cloudCommand = doc["command"]; 

          // ------ 📡 BACK-FORWARDING LOOP TO NODE 1 ------
          if (cloudCommand != NULL) {
            LoRa.beginPacket();
            LoRa.print(cloudCommand);
            LoRa.endPacket();
            Serial.print("📡 Radio Downlink sent Cloud Decision to Node 1: "); 
            Serial.println(cloudCommand);
          }
        }
      } else {
        Serial.print("❌ Cloud API Connection Error: "); 
        Serial.println(http.errorToString(httpCode).c_str());
      }
      http.end(); 
    } else {
      Serial.println("📶 Wi-Fi Disconnected! Reconnecting...");
    }
  }
}
