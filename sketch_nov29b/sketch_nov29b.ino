#include <SPI.h>
#include <RF24.h>
#include <SoftPWM.h>

#define CE_PIN  10
#define CSN_PIN  9

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1Node";          // Same address on BOTH boards
const unsigned long RF_PERIOD = 30; 
unsigned long rfTimer = 0;

struct Payload {
  uint8_t nodeId;
  uint16_t value;
};
Payload payload;


void setup() {
  Serial.begin(9600);
  pinMode(0, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);

  radio.begin();

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);       // 250 kbps = best range & reliability
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(115);

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
    Serial.print(F(" → value="));
    Serial.println(payload.value);
  }

  /*Serial.print(analogRead(A0));
  Serial.print(" ");
  Serial.print(analogRead(A1));
  Serial.print(" ");
  Serial.print(analogRead(A2));
  Serial.print(" ");
  Serial.print(analogRead(A3));
  Serial.print(" ");
  Serial.print(analogRead(A6));
  Serial.print(" ");
  Serial.print(analogRead(A7));
  Serial.print(" ");
  Serial.print(digitalRead(0));
  Serial.print(" ");
  Serial.print(digitalRead(2));
  Serial.print(" ");
  Serial.print(digitalRead(3));
  Serial.print(" ");
  Serial.print(digitalRead(4));
  Serial.print(" ");
  Serial.print(digitalRead(7));
  Serial.print(" ");
  Serial.println(digitalRead(8));*/

  analogWrite(5, analogRead(A0) / 4);
  analogWrite(6, analogRead(A6) / 4);


  unsigned long now = millis();
  // 2. TRANSMIT PART (every 100 ms)
  if (now - rfTimer >= RF_PERIOD){
    rfTimer = now;
    radio.stopListening();              // switch to TX mode

    payload.nodeId = 1;                 // Change to 2 on the other board!
    payload.value = analogRead(A7);

    bool ok = radio.write(&payload, sizeof(payload));

    Serial.print(F("Sent "));
    Serial.print(payload.value);
    Serial.println(ok ? F(" → OK") : F(" → FAILED"));

    radio.startListening();             // back to RX mode
  }
}
