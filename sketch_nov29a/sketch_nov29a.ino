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
const unsigned long RF_PERIOD = 500; 
unsigned long rfTimer = 0;

struct Payload {
  uint8_t nodeId;
  uint16_t value;
};
Payload payload;

VL53L1X sensor;
MPU6050 mpu;
unsigned long gyroTimer = 0;
const unsigned long GYRO_PERIOD = 10;   // 100 Hz

uint8_t servo1Value;
uint8_t servo2Value;
uint8_t value = 0;

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
uint8_t masterCallIntArray(uint8_t func, const uint8_t* args = nullptr, uint8_t len = 0, uint16_t* results = nullptr, uint8_t resultslen = 0) {
  Wire.beginTransmission((uint8_t)SLAVE_ADDR);
  Wire.write(func);
  if (args && len == 8) Wire.write(args, len);
  Wire.endTransmission();

  if (Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)16) != 16 || resultslen != 16) return 254;

  uint8_t buffer[16];
  Wire.readBytes(buffer, 16);

  for (uint8_t i = 0; i < 8; i++) {
    results[i] = buffer[i*2] | (buffer[i*2+1] << 8);  // little-endian
  }
  return 0;
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

  sensor.setTimeout(100);
  sensor.init();
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);
  sensor.startContinuous(50);

  for (int i = 4; i < 8; i++) {
    SoftPWMSet(i, 0);
  }
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
    radio.read(&payload, sizeof(payload));

    Serial.print(F("Received from node "));
    Serial.print(payload.nodeId);
    Serial.print(F(" → value="));
    Serial.println(payload.value);
  }

  Serial.println(sensor.read());  
  //Serial.println(masterCallInt(7, B(0,1), 2));
  //Serial.println(masterCallInt(7, B(2,3), 2));
  //Serial.print(analogRead(A7));

  uint16_t values[8];
  masterCallIntArray(5, B(10, 11, 12, 13, 14, 15, 16, 17), 8, values, sizeof(values));
  Serial.print(values[0]);
  Serial.print(values[1]);
  Serial.print(values[2]);
  Serial.print(values[3]);
  Serial.print(values[4]);
  Serial.print(values[5]);
  Serial.print(values[6]);
  Serial.println(values[7]);

  if(value%2 == 0)
    masterCallInt(9, B(5, 180), 2);
  else
    masterCallInt(9, B(180, 180), 2);
  motor(4,5,1,60);
  value++;

  unsigned long now = millis();
  if (now - gyroTimer >= GYRO_PERIOD){
      gyroTimer = now;
      Vector norm = mpu.readNormalizeGyro();
      pitch += norm.YAxis * 0.01;
      roll  += norm.XAxis * 0.01;
      yaw   += norm.ZAxis * 0.01;

      // Output raw
      Serial.print(" Pitch = ");
      Serial.print(pitch);
      Serial.print(" Roll = ");
      Serial.print(roll);  
      Serial.print(" Yaw = ");
      Serial.println(yaw);
  }

  // 2. TRANSMIT PART (every 100 ms)
  if (now - rfTimer >= RF_PERIOD){
    rfTimer = now;
    radio.stopListening();              // switch to TX mode

    payload.nodeId = 2;                 // Change to 2 on the other board!
    payload.value = analogRead(A7);

    bool ok = radio.write(&payload, sizeof(payload));

    Serial.print(F("Sent "));
    Serial.print(payload.value);
    Serial.println(ok ? F(" → OK") : F(" → FAILED"));

    radio.startListening();             // back to RX mode
  }
}