#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"


#define SCK     18
#define MISO    19
#define MOSI    23
#define SS      5
#define RST     14
#define DIO0    26

// --- Sensors & LED Pins ---
#define DHTPIN 27          
#define DHTTYPE DHT22      
#define MQ135_PIN 34       
#define LED_PIN 2     // Onboard Blue LED ya Mist Spray Relay Pin

DHT dht(DHTPIN, DHTTYPE);


struct _attribute((packed_)) DataPacket {
  float temperature;
  float humidity;
  int gasValue;
  int dustValue;      
  bool sprayStatus;   
};

DataPacket packet;

// Non-blocking timer variables
unsigned long lastTxTime = 0;
const unsigned long txInterval = 5000; 

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 
  packet.sprayStatus = false;

  dht.begin();
  randomSeed(analogRead(0)); 

  // SPI and LoRa hardware initialization
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(866E6)) { 
    Serial.println("❌ Node 1: LoRa init fail ho gaya!");
    while (1);
  }
  
  Serial.println("==================================================");
  Serial.println("🚀 NODE 1 SYSTEM READY (Bidirectional Cloud Mode)");
  Serial.println("==================================================");
}

void loop() {
  
  // === STEP 1: S NODE SE CLOUD ENGINE KA DOWNLINK ORDER SUNNA ===
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      char incomingCommand = (char)LoRa.read();
      
      // Cloud Command 1: Hotspot detected -> Turn ON Purifier
      if (incomingCommand == '1') {
        digitalWrite(LED_PIN, HIGH); 
        packet.sprayStatus = true;
        Serial.println("\n📥 [Cloud Command Recv]: [1] -> Activating Mist Spray Purifier! 🔴");
      } 
      
      else if (incomingCommand == '0') {
        digitalWrite(LED_PIN, LOW);  
        packet.sprayStatus = false;
        Serial.println("\n📥 [Cloud Command Recv]: [0] -> Deactivating Mist Spray Purifier. Standby... 🟢");
      }
    }
  }


  if (millis() - lastTxTime >= txInterval) {
    lastTxTime = millis();

    
    packet.humidity = dht.readHumidity();
    packet.temperature = dht.readTemperature();
    packet.gasValue = analogRead(MQ135_PIN);
    
   
    packet.dustValue = random(180, 420); 

  
    if (isnan(packet.humidity) || isnan(packet.temperature)) {
      packet.humidity = 0.0;
      packet.temperature = 0.0;
    }

    
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&packet, sizeof(packet));
    LoRa.endPacket();

    // Serial monitor display logging
    Serial.println("--------------------------------------------------");
    Serial.println("📤 [Uplink Sent to Cloud via S Node]");
    Serial.print("🌡️ Temp: "); Serial.print(packet.temperature); Serial.print(" °C");
    Serial.print(" | 💧 Humid: "); Serial.print(packet.humidity); Serial.println(" %");
    Serial.print("💨 MQ135 Gas Raw: "); Serial.print(packet.gasValue);
    Serial.print(" | 🌫️ Dust: "); Serial.print(packet.dustValue);
    Serial.print(" | 🔌 Purifier State: "); Serial.println(packet.sprayStatus ? "ON" : "OFF");
    Serial.println("--------------------------------------------------");
  }
}