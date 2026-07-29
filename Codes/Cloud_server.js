// 🔴 AUTOMATIC INTEGRATED CREDENTIALS BY CLOUD SERVER
var FIREBASE_URL = "https://lora-aqi-system-default-rtdb.firebaseio.com/nodes/node_1.json?auth=alDS4QLcqzO6lXqeSTnxRi5axBtORZihDCH48AcW";
var TELEGRAM_BOT_TOKEN = "8314838505:AAHESornCy0weCgKAg71RxtUVCiH03mtvCo";
var TELEGRAM_CHAT_ID = "-1004321229754";

function doPost(e) {
  try {
    var jsonInput = JSON.parse(e.postData.contents);
    var temp = jsonInput.temperature;
    var humid = jsonInput.humidity;
    var gas = jsonInput.gasValue;
    var dust = jsonInput.dustValue;  // Database logging active, calculation ignored
    var lastState = jsonInput.lastState;

    // ------ 🧠 1. REAL-WORLD CALIBRATED GAS AQI FORMULA ------
    // Baseline: 200, Hotspot Target: 800 raw value = 150 AQI
    var calculatedAQI = 10;
    
    if (gas > 200) {
      calculatedAQI = ((gas - 200) * (150 - 10) / (800 - 200)) + 10;
    }
    calculatedAQI = Math.round(calculatedAQI);
    
    // Bounds Control
    if (calculatedAQI < 10) calculatedAQI = 10;
    if (calculatedAQI > 500) calculatedAQI = 500;

    // ------ ☁️ 2. STORE DATA IN FIREBASE ------
    var payload = {
      "temperature": temp,
      "humidity": humid,
      "gasRaw": gas,
      "dustRaw": dust, 
      "calculatedAQI": calculatedAQI
    };
    
    var options = {
      "method": "put",
      "contentType": "application/json",
      "payload": JSON.stringify(payload)
    };
    UrlFetchApp.fetch(FIREBASE_URL, options);

    // ------ ⚡ 3. HOTSPOT & RECOVERY DEBOUNCED LOOP ------
    var command = lastState.toString(); 
    
    // Condition A: Actual Smoke Hotspot Trigger
    if (calculatedAQI >= 150) { 
      command = "1"; 
      if (lastState == 0) { 
        sendTelegram("⚠️ [HOTSPOT DETECTED VIA CLOUD] \n\n📍 Location: Node 1 Area\n💨 Calculated AQI: " + calculatedAQI + " (UNSAFE)\n🌡️ Temp: " + temp + "°C\n\n📢 Action: Cloud Server has issued a downlink command to activate the Mist Spray Purifier!");
      }
    } 
    // Condition B: Safe Environment Recovery
    else if (calculatedAQI < 120) { 
      command = "0"; 
      if (lastState == 1) { 
        sendTelegram("✅ [ENVIRONMENT SAFE VIA CLOUD] \n\n📍 Location: Node 1 Area\n💨 Calculated AQI: " + calculatedAQI + " (SAFE Limit)\n\n📢 Action: Air quality restored. Turning OFF the Purifier.");
      }
    }

    var response = { "command": command };
    return ContentService.createTextOutput(JSON.stringify(response)).setMimeType(ContentService.MimeType.JSON);

  } catch(error) {
    return ContentService.createTextOutput(JSON.stringify({"error": error.toString()})).setMimeType(ContentService.MimeType.JSON);
  }
}

function sendTelegram(text) {
  var url = "https://api.telegram.org/bot" + TELEGRAM_BOT_TOKEN + "/sendMessage?chat_id=" + TELEGRAM_CHAT_ID + "&text=" + encodeURIComponent(text);
  UrlFetchApp.fetch(url);
}