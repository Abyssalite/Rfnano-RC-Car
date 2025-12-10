#include <Wire.h>
#define SLAVE_ADDR 8
#define B(...)  (const uint8_t[]){ __VA_ARGS__ }

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

void setup() {
  Wire.begin();
  Serial.begin(9600);

  masterCallInt(2, B(1, 7), 2);
  masterCallInt(2, B(0, 8, 14), 3);
  masterCallInt(8, B(5, 6), 2);
}

void loop() {
  Serial.println(masterCallInt(7, B(7,8), 2));

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

  //Serial.println(masterCallInt(4, B(14), 1));
  //masterCallInt(9, B(0, 180), 2);
  //delay(200);
  //masterCallInt(9, B(180, 0), 2);
  delay(200);
}
