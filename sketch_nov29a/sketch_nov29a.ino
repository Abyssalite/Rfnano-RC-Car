#include <Wire.h>
#define SLAVE_ADDR 8
#define B(...)  (const uint8_t[]){ __VA_ARGS__ }
#include <RF24.h>
#include <SoftPWM.h>
#include <MPU6050.h>
#include <VL53L1X.h>

#define CE_PIN  10
#define CSN_PIN  9
RF24 radio(CE_PIN, CSN_PIN);

const byte address[6] = "1Node";          // Same address on BOTH boards
const uint8_t RF_PERIOD = 300; 
unsigned long rfTimer = 0;

const uint8_t FRONT_LEFT[2] = {A1, A0}; 
const uint8_t FRONT_RIGHT[2] = {8, 7};  
const uint8_t REAR_LEFT[2] = {A3, A2};  
const uint8_t REAR_RIGHT[2] = {4, 2}; ; 

struct SendPayload {
    uint8_t digitalIR[4];
    uint8_t analogIR[4];
    uint16_t distanceSensor[3];
    int16_t gyro[3];
    uint8_t batt;
};
SendPayload sendPayload;
struct ReceivePayload {
    int8_t joystick1[2];
    int8_t joystick2[2];
    uint8_t digitalButton[6];
    uint8_t analogButton;
    uint8_t batt;
};
ReceivePayload receivePayload;

VL53L1X sensor;
MPU6050 mpu;
unsigned long gyroTimer = 0;
const uint8_t GYRO_PERIOD = 10;   // 100 Hz

uint8_t servo1Value;
uint8_t servo2Value;

// Pitch, Roll and Yaw values
float pitch = 0;
float roll = 0;
float yaw = 0;

// int return
uint16_t masterCallInt(uint8_t func, const uint8_t* args = nullptr, uint8_t len = 0) {
  Wire.beginTransmission((uint8_t)SLAVE_ADDR);
  Wire.write(func);
  if (args && len > 0) Wire.write(args, len);
  Wire.endTransmission();

  if (Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)2) != 2) return 2254;

  uint16_t value;
  Wire.readBytes((uint8_t*)&value, 2);

  return value;
}

// 8-int return
uint8_t masterCallIntArray(uint8_t func, const uint8_t* args = nullptr, uint8_t len = 0, uint8_t* results = nullptr, uint8_t resultslen = 0) {
  Wire.beginTransmission((uint8_t)SLAVE_ADDR);
  Wire.write(func);
  if (args && len == 8) Wire.write(args, len);
  Wire.endTransmission();

  if (Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)8) != 8 || resultslen != 8) return 254;

  uint8_t buffer[8];
  Wire.readBytes(buffer, 8);

  for (uint8_t i = 0; i < 8; i++) {
    results[i] = buffer[i] | (buffer[i+1] << 8);  // little-endian
  }
  return 0;
}

void motor(int pin1, int pin2, int& speed) {
  speed = map(speed, -126, 126, -100, 100);
  speed = constrain(speed, -100, 100);

  if (speed > 10) {
    SoftPWMSetPercent(pin2, 0);
    SoftPWMSetPercent(pin1, speed);
  } 
  else if (speed < -10) {
    SoftPWMSetPercent(pin1, 0);
    SoftPWMSetPercent(pin2, abs(speed));
  }
  else {
    SoftPWMSetPercent(pin1, 0);
    SoftPWMSetPercent(pin2, 0);
  }
}

void moveFordward(int speed){
  motor(FRONT_LEFT[0],FRONT_LEFT[1],speed);
  motor(FRONT_RIGHT[0],FRONT_RIGHT[1],speed);
  motor(REAR_LEFT[0],REAR_LEFT[1],speed);
  motor(REAR_RIGHT[0],REAR_RIGHT[1],speed);
}

void setup() {  
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C
  SoftPWMBegin();  
  radio.begin();

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);       // 250 kbps = best range & reliability
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(115);

  radio.openWritingPipe(address);       // TX address
  radio.openReadingPipe(1, address);    // RX address (different pipe)

  radio.startListening();               // start in RX mode
  Serial.println(F("Bidirectional node ready"));

  Serial.println(mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G));
  // Calibrate gyroscope. The calibration must be at rest.
  mpu.calibrateGyro();
  mpu.setThreshold(3);

  sensor.init();
  sensor.setTimeout(50);
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(33000);
  sensor.startContinuous(50);

  SoftPWMSet(0, 0);
  SoftPWMSet(4, 0);
  SoftPWMSet(7, 0);
  SoftPWMSet(8, 0);
  SoftPWMSet(A0, 0);
  SoftPWMSet(A1, 0);
  SoftPWMSet(A2, 0);
  SoftPWMSet(A3, 0);

  masterCallInt(2, B(0, 1, 3), 3);
  masterCallInt(2, B(1, 0, 2), 3);
  masterCallInt(8, B(5, 6), 2);
}

void loop() {
  // 1. RECEIVE PART (always active)
  if (radio.available()) {
    radio.read(&receivePayload, sizeof(receivePayload));
    
    Serial.print(receivePayload.joystick1[0]);
    Serial.print(" ");
    Serial.print(receivePayload.joystick1[1]);
    Serial.print(" ");
    Serial.print(receivePayload.joystick2[0]);
    Serial.print(" ");
    Serial.print(receivePayload.joystick2[1]);
    Serial.print(" ");
    Serial.print(map(receivePayload.analogButton, 0, 255, 0, 600));
    Serial.print(" ");
    Serial.print(map(receivePayload.batt, 0, 255, 0, 1023));
    Serial.print(" ");

    Serial.print(receivePayload.digitalButton[0]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalButton[1]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalButton[2]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalButton[3]);
    Serial.print(" ");
    Serial.print(receivePayload.digitalButton[4]);
    Serial.print(" ");
    Serial.println(receivePayload.digitalButton[5]);
  }

  const uint16_t sensors[3] = {sensor.read(), masterCallInt(7, B(0,1), 2), masterCallInt(7, B(2,3), 2)};
  uint8_t values[8];
  masterCallIntArray(5, B(10, 11, 12, 13, 14, 15, 16, 17), 8, values, sizeof(values));

unsigned long now = millis();
  if (now - gyroTimer >= GYRO_PERIOD){
      gyroTimer = now;
      Vector norm = mpu.readNormalizeGyro();
      pitch += norm.YAxis * 0.01;
      roll  += norm.XAxis * 0.01;
      yaw   += norm.ZAxis * 0.01;

      pitch = (abs(pitch) > 18.0)? pitch * -1 : pitch;
      roll = (abs(roll) > 18.0)? roll * -1 : roll;
      yaw = (abs(yaw) > 18.0)? yaw * -1 : yaw;

      sendPayload.gyro[0] = (int16_t)(pitch * 100.0f);
      sendPayload.gyro[1] = (int16_t)(roll * 100.0f);
      sendPayload.gyro[2] = (int16_t)(yaw * 100.0f);
  }

  /*masterCallInt(9, B(5, 180), 2);
  masterCallInt(9, B(180, 180), 2);*/
  
  moveFordward(receivePayload.joystick2[1]);

  // 2. TRANSMIT PART (every 100 ms)
  if (now - rfTimer >= RF_PERIOD){
    rfTimer = now;
    radio.stopListening();              // switch to TX mode

    sendPayload.digitalIR[0] = values[0];
    sendPayload.digitalIR[1] = values[1];
    sendPayload.digitalIR[2] = values[2];
    sendPayload.digitalIR[3] = values[3];

    sendPayload.analogIR[0] = values[4];
    sendPayload.analogIR[1] = values[5];
    sendPayload.analogIR[2] = values[6];
    sendPayload.analogIR[3] = values[7];

    sendPayload.distanceSensor[0] = sensors[0];
    sendPayload.distanceSensor[1] = sensors[1];
    sendPayload.distanceSensor[2] = sensors[2];

    sendPayload.batt = map(analogRead(A7), 0, 1023, 0, 255);

    bool ok = radio.write(&sendPayload, sizeof(sendPayload));

    //Serial.print(F("Sent"));
    //Serial.println(ok ? F(" → OK") : F(" → FAILED"));

    radio.startListening();             // back to RX mode
  }
}