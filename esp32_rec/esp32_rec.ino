// ESP1 Receiver Code
// #include <esp_sleep.h>

const int receivePin = 5;       // GPIO pin for receiving wake-up and message
const int ledPin = 2;           // GPIO pin for LED
const int ackPin = 4;           // GPIO pin to send acknowledgment
const int bitDelay = 500;       // Should match ESP0's bit delay time
const int pulseDelay = 200;     // Should match ESP0's pulse delay for preamble
const byte targetMessage = 0b0001;  // Target message for this ESP1
unsigned long lastMessageTime = 0;  // Tracks the last time a message was received

void setup() {
  pinMode(receivePin, INPUT);
  pinMode(ackPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  Serial.begin(115200);
  digitalWrite(ackPin, LOW);    // Ensure ackPin is LOW initially

  // Set up deep sleep to wake on receivePin
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, HIGH); // Wake up when receivePin goes HIGH
}

bool detectPreamble() {
  int highCount = 0;
  for (int i = 0; i < 4; i++) {
    if (digitalRead(receivePin) == HIGH) {
      highCount++;
      delay(pulseDelay);  // Wait for the pulse duration
    } else {
      return false;       // If a LOW is detected early, fail
    }
  }

  if (digitalRead(receivePin) == LOW) {
    delay(pulseDelay);  // Allow for stability
    return true;
  }
  
  return false;
}

byte readMessage() {
  byte message = 0;
  for (int i = 3; i >= 0; i--) {
    int signal = digitalRead(receivePin);
    if (signal == HIGH) {
      message |= (1 << i);
    }
    delay(bitDelay);
  }
  return message;
}

void sendAck() {
  digitalWrite(ackPin, HIGH);
  delay(500);               // Short HIGH pulse as acknowledgment
  digitalWrite(ackPin, LOW);
}

void checkSleep() {
  if (millis() - lastMessageTime > 60000) {  // 1 minute inactivity
    Serial.println("Entering deep sleep.");
    delay(100);  // Small delay before sleeping
    esp_deep_sleep_start();
  }
}

void loop() {
  // checkSleep();  // Check if ESP1 should enter deep sleep

//  if (digitalRead(receivePin) == HIGH) {  // Wake-up signal received
//    delay(500);  // Small delay to stabilize after wake-up signal
//    Serial.println("Woke up and ready to receive message.");
//  }

  if (detectPreamble()) {
    byte receivedMessage = readMessage();

    if (receivedMessage == targetMessage) {
      digitalWrite(ledPin, HIGH);
      Serial.println("Message received for ESP1!");
      sendAck();
      delay(1000);                // Keep LED on for 1 second
      digitalWrite(ledPin, LOW);  // Turn off LED
      lastMessageTime = millis();  // Reset inactivity timer
    } else {
      Serial.println("Message not for ESP1.");
    }
  }

  delay(500);  // Small delay before next loop iteration to avoid flooding
}
