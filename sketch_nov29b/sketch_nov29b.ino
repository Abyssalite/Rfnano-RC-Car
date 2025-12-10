#include <SPI.h>
#include <RF24.h>
#include <SoftPWM.h>

#define CE_PIN  10
#define CSN_PIN  9

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1Node";          // Same address on BOTH boards
const unsigned long interval = 1000;

struct Payload {
  uint8_t nodeId;
  uint8_t counter;
  char    letter;
};
Payload payload;

unsigned long lastSend = 0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  // motor pwm pin
  SoftPWMBegin();

  for (int i = 2; i < 8; i++) {
      SoftPWMSet(i, 0);
      SoftPWMSetFadeTime(i, 100, 100);
  }
  SoftPWMSet(14, 0);
  SoftPWMSet(15, 0);
  SoftPWMSetFadeTime(14, 100, 100);
  SoftPWMSetFadeTime(15, 100, 100);

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);       // 250 kbps = best range & reliability
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(115);

  radio.openWritingPipe(address);       // TX address
  radio.openReadingPipe(1, address);    // RX address (different pipe)

  radio.startListening();               // start in RX mode
  Serial.println(F("Bidirectional node ready"));
}

void motor(int pin1, int pin2, int direction, int speed) {
  speed = constrain(speed, 0, 100);

  if (direction == 1) {
    SoftPWMSetPercent(pin2, 0);
    SoftPWMSetPercent(pin1, speed);
  } 
  else if (direction == -1) {
    SoftPWMSetPercent(pin1, 0);
    SoftPWMSetPercent(pin2, speed);
  }
  else {
    SoftPWMSetPercent(pin1, 0);
    SoftPWMSetPercent(pin2, 0);
  }
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

    radio.startListening();             // back to RX mode
  }

  motor(14, 15, -1, 25);  // D reverse
  delay(1000);

  motor(14, 15, 0, 0);  // D reverse
  delay(1000);

  motor(14, 15, 1, 25);  // D reverse
  delay(1000); 
}
