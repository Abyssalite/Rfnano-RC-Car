#include <SPI.h>
#include <RF24.h>

#define CE_PIN  10
#define CSN_PIN  9

RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1Node";          // Same address on BOTH boards
const uint8_t RF_PERIOD = 30; 
unsigned long rfTimer = 0;

struct SendPayload {
    int8_t joystick1[2];
    int8_t joystick2[2];
    uint8_t digitalButton[6];
    uint8_t analogButton;
    uint8_t batt;
};
SendPayload sendPayload;
struct ReceivePayload {
    uint8_t digitalIR[4];
    uint8_t analogIR[4];
    uint16_t distanceSensor[3];
    int16_t gyro[3];
    uint8_t batt;
};
ReceivePayload receivePayload;

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
    radio.read(&receivePayload, sizeof(receivePayload));

    Serial.print(receivePayload.distanceSensor[0]);
    Serial.print(" ");
    Serial.print(receivePayload.distanceSensor[1]);
    Serial.print(" ");
    Serial.print(receivePayload.distanceSensor[2]);
    Serial.print(" ");
    Serial.print(receivePayload.gyro[0]);
    Serial.print(" ");
    Serial.print(receivePayload.gyro[1]);
    Serial.print(" ");
    Serial.print(receivePayload.gyro[2]);
    Serial.print(" ");
    Serial.print(map(receivePayload.batt, 0, 255, 0, 1023));
    Serial.print(" ");

    Serial.print(receivePayload.digitalIR[0]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalIR[1]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalIR[2]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalIR[3]);
    Serial.print(" ");
    Serial.print(receivePayload.analogIR[0]);
    Serial.print(" ");
    Serial.print(receivePayload.analogIR[1]);
    Serial.print(" ");
    Serial.print(receivePayload.analogIR[2]);
    Serial.print(" ");
    Serial.println(receivePayload.analogIR[3]);
  }

  //analogWrite(5, analogRead(A0) / 4);
  //analogWrite(6, analogRead(A6) / 4);

  unsigned long now = millis();
  // 2. TRANSMIT PART (every 100 ms)
  if (now - rfTimer >= RF_PERIOD){
    rfTimer = now;
    radio.stopListening();              // switch to TX mode

    sendPayload.joystick1[0] = map(analogRead(A0), 0, 740, 0, 255) - 120;
    sendPayload.joystick1[1] = map(analogRead(A1), 0, 740, 0, 255) - 124;

    sendPayload.joystick2[0] = map(analogRead(A2), 0, 740, 0, 255) - 122;
    sendPayload.joystick2[1] = map(analogRead(A3), 0, 740, 0, 255) - 125; 

    sendPayload.digitalButton[0] = digitalRead(0);
    sendPayload.digitalButton[1] = digitalRead(2);
    sendPayload.digitalButton[2] = digitalRead(3);
    sendPayload.digitalButton[3] = digitalRead(4);
    sendPayload.digitalButton[4] = digitalRead(7);
    sendPayload.digitalButton[5] = digitalRead(8);

    sendPayload.analogButton = map(analogRead(A6), 0, 600, 0, 255);
    sendPayload.batt = map(analogRead(A7), 0, 1023, 0, 255);

    bool ok = radio.write(&sendPayload, sizeof(sendPayload));

    //Serial.print(F("Sent"));
    //Serial.println(ok ? F(" → OK") : F(" → FAILED"));

    radio.startListening();             // back to RX mode
  }
}
