#include <Arduino.h>
#include "EspNowStream.h"

#define LED_PIN   2
#ifdef LED_PIN
#define LED_LEVEL LOW
#endif

#define ESP_NOW_CHANNEL 14

//static const uint8_t MAC[6] PROGMEM = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ! LED_LEVEL);
#endif

  if (! EspNowStream.begin(ESP_NOW_CHANNEL)) {
//  if (! EspNowStream.begin(ESP_NOW_CHANNEL, MAC)) {
    Serial.println(F("ESP-NOW initialization fail!"));
    Serial.flush();

    ESP.deepSleep(0);
  }
}

void loop() {
  const uint32_t SEND_PERIOD = 500; // 500 ms.

  static uint32_t lastSend = 0;

  if (EspNowStream.available()) {
#ifdef LED_PIN
    digitalWrite(LED_PIN, LED_LEVEL);
#endif
    while (EspNowStream.available()) {
      Serial.write(EspNowStream.read());
    }
#ifdef LED_PIN
    digitalWrite(LED_PIN, ! LED_LEVEL);
#endif
  }

  if ((! lastSend) || (millis() - lastSend >= SEND_PERIOD)) {
    static uint8_t cnt = 0;

#ifdef LED_PIN
    digitalWrite(LED_PIN, LED_LEVEL);
#endif
    EspNowStream.println(cnt++);
#ifndef AUTO_FLUSH_TIME
    EspNowStream.flush();
#endif
#ifdef LED_PIN
    digitalWrite(LED_PIN, ! LED_LEVEL);
#endif
    lastSend = millis();
  }
}
