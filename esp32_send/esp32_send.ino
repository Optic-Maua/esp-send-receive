// ESP0 Sender Code
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

const int sendPin = 5;           // GPIO pin for sending message
const int ackPin = 4;            // GPIO pin to receive acknowledgment
const int bitDelay = 500;        // Delay per bit in milliseconds
const int pulseDelay = 200;      // Delay for each pulse in preamble
const int interMessageDelay = 2000;  // Delay between messages
const int ackTimeout = 1000;     // Time to wait for acknowledgment in ms

int lineId = 0;

const char* ssid = "Bau_chicka_bau_uau";
const char* password = "EdnaldoPereira78";

void setup() {
  pinMode(sendPin, OUTPUT);
  pinMode(ackPin, INPUT);  
  
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wi-Fi...");

  unsigned long startAttemptTime = millis();

  // Keep trying to connect for 10 seconds (10000 ms)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi");
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
  }
  
}

//void sendWakeUp() {
//  digitalWrite(sendPin, HIGH);
//  delay(500);                  // Send a 500ms pulse to wake up ESP1
//  digitalWrite(sendPin, LOW);
//  delay(500);                  // Allow ESP1 to stabilize after wake-up
//}

void sendPreamble() {
  for (int i = 0; i < 4; i++) {  
    digitalWrite(sendPin, HIGH);
    delay(pulseDelay);
  }
  digitalWrite(sendPin, LOW);  
  delay(pulseDelay);           
}

void sendMessage(byte message) {
  for (int i = 7; i >= 0; i--) {
    bool bitValue = (message >> i) & 0x01;
    digitalWrite(sendPin, bitValue ? HIGH : LOW);
    delay(bitDelay);
  }
}

bool waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ackTimeout) {
    if (digitalRead(ackPin) == HIGH) {
      delay(10);  // Small debounce delay
      return true;
    }
  }
  return false;
}

void sendPost(int targetId, String currentStatus) {
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Specify the URL
    String url = "https://admin-dashboard-silk-gamma.vercel.app/api/change-status?id=" + String(targetId) + "&status=" + String(currentStatus);
    http.begin(url);  // URL to send the POST request to
    // Serial.println(url);
    

    // Set the content type and other headers if needed
    //http.addHeader("Content-Type", "application/json");

    // Your JSON data or payload
    String payload = "";

    // Send POST request
    int httpResponseCode = http.POST(payload);

    // Check the response code
    if (httpResponseCode > 0) {
      String response = http.getString();  // Get the response to the request
      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);
      Serial.print("Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    // Close the connection
    http.end();
  } else {
    Serial.println("Wi-Fi disconnected");
  }
}

void sendWithRetry(byte message) {
  int targetId = message;  // Convert byte to int for target ID in decimal
  for (int attempt = 0; attempt < 2; attempt++) {
    sendPreamble();
    sendMessage(message);
    digitalWrite(sendPin, LOW);  // End signal
    delay(bitDelay);             // Short delay before checking ack

    if (waitForAck()) {
      Serial.print("Acknowledgment received from ESP");
      Serial.println(targetId);
      sendPost(targetId, "ON");
      return;  // Stop retrying if acknowledgment is received
    } else {
      Serial.print("No acknowledgment from ESP");
      Serial.print(targetId);
      Serial.println(". Retrying...");
    }
  }

  // If both attempts failed
  sendPost(targetId, "OFF");
}

std::vector<int> getIdsFromLine(int lineId) {
  std::vector<int> idArray;  // Array to store the IDs

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://admin-dashboard-silk-gamma.vercel.app/api/line?line=" + String(lineId));
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) { // Check for successful response
      String responseBody = http.getString();
      Serial.println("Response:");
      Serial.println(responseBody);

      // Parse JSON response
      DynamicJsonDocument doc(1024);  // Adjust size based on expected JSON size
      DeserializationError error = deserializeJson(doc, responseBody);

      if (!error) {
        JsonArray ids = doc.as<JsonArray>();

        // Populate the vector with IDs from the JSON array
        for (int id : ids) {
          idArray.push_back(id);
        }
      } else {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Close connection
  } else {
    Serial.println("WiFi not connected");
  }

  return idArray;  // Return the vector of IDs
}

byte idToByte(int id) {
  // Truncate or wrap the ID to fit within 8 bits
  return static_cast<byte>(id); 
}

void loop() {

  int lineId = 3;

  std::vector<int> ids = getIdsFromLine(lineId);

  for (size_t i = 0; i < ids.size(); i++) {
    byte message = idToByte(ids[i]);
    sendWithRetry(message);
  }

  delay(60000);
}
