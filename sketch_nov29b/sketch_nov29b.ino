#include <SPI.h>
#include <RF24.h>

#define CE_PIN  10
#define CSN_PIN  9

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1Node";          // Same address on BOTH boards
const unsigned long interval = 5000;

struct Payload {
  uint8_t nodeId;
  uint8_t counter;
  char    letter;
};
Payload payload;

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  radio.begin();

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);       // 250 kbps = best range & reliability
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(115);

  // VERY IMPORTANT — both boards use the SAME address on pipe 0
  radio.openWritingPipe(address);       // TX address
  radio.openReadingPipe(1, address);    // RX address (different pipe)

  radio.startListening();               // start in RX mode
  Serial.println(F("Bidirectional node ready"));
}

void loop() {
  // 1. RECEIVE PART (always active)
  if (radio.available()) {
    radio.read(&payload, sizeof(payload));

    Serial.print(F("Received from node "));
    Serial.print(payload.nodeId);
    Serial.print(F(" → counter="));
    Serial.print(payload.counter);
    Serial.print(F(" letter="));
    Serial.println(payload.letter);
  }

  // 2. TRANSMIT PART (every 500 ms)
  if (millis() - lastSend >= interval) {
    lastSend = millis();

    radio.stopListening();              // switch to TX mode

    payload.nodeId = 2;                 // Change to 2 on the other board!
    payload.counter++;
    payload.letter = 'A' + (payload.counter % 26);

    bool ok = radio.write(&payload, sizeof(payload));

    Serial.print(F("Sent "));
    Serial.print(payload.counter);
    Serial.println(ok ? F(" → OK") : F(" → FAILED"));

    radio.startListening();             // immediately back to RX mode
  }
}
