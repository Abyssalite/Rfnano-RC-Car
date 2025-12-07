#include <Wire.h>
#define SLAVE_ADDR 8
byte args[] = {1, 7, 8};


// 1-byte return
byte masterCallByte(byte func, const byte* args = nullptr, int len = 0) {
  Wire.beginTransmission((byte)SLAVE_ADDR);
  Wire.write(func);
  if (args && len > 0) Wire.write(args, len);
  Wire.endTransmission();

  Wire.requestFrom((byte)SLAVE_ADDR, (byte)1);
  return Wire.available() ? Wire.read() : 255;
}

// 8-int return
void masterCallIntArray(byte func, const byte* args = nullptr, int len = 0, int result[8] = nullptr) {
  Wire.beginTransmission((byte)SLAVE_ADDR);
  Wire.write(func);
  if (args && len > 0) Wire.write(args, len);
  Wire.endTransmission();

  Wire.requestFrom((byte)SLAVE_ADDR, (byte)16);
  byte buffer[16];
  Wire.readBytes(buffer, 16);

  for (int i = 0; i < 8; i++) {
    result[i] = buffer[i*2] | (buffer[i*2+1] << 8);  // little-endian
  }
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
  byte x = masterCallByte(2, args, sizeof(args));
  Serial.println(x);
}

void loop() {
  args[0] = 1;
  byte x = masterCallByte(3, args, sizeof(args));
  delay(100);
  args[0] = 0;
  x = masterCallByte(3, args, sizeof(args));
  delay(100);

  //byte args1[8];
  //int values[8];
  //masterCallIntArray(4, args1, sizeof(args1), values);


}
