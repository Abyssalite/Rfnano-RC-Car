#include <Wire.h>

#define SLAVE_ADDR 8 

// Store received data temporarily
byte args[16] = {0};  // max arguments
byte argCount = 0;
union {
  byte     singleByte;
  int      intArray[8];
} result;
bool returnSingleByte = true;

void setup() {
  Wire.begin((byte)SLAVE_ADDR);
  Wire.onReceive(receiveEvent);  // When master sends data
  Wire.onRequest(requestEvent);  // When master requests data
}

void loop() {
}

void receiveEvent(int receive) {
  if (receive < 1) return;

  byte functionId = Wire.read();  // First byte is function number
  argCount = receive - 1;
  
  // Read all remaining bytes as arguments
  for (byte i = 0; i < argCount && Wire.available(); i++) {
    args[i] = Wire.read();
  }
  returnSingleByte = true;

  switchFunction(&functionId);
}

void readPin() {
  for (byte i = 0; i < argCount ; i++) {
    if (args[i] < 14)
      result.intArray[i] = digitalRead(args[i]); 
    else if (args[i] < 18)
      result.intArray[i] = analogRead(args[i]); 
  }
}

void writePin() {
  bool value = args[0];
  for (byte i = 1; i <= argCount - 1 ; i++) {
    if (args[i] < 18)
      digitalWrite(args[i], value);
  }

  result.singleByte = 3;
}


void setPin() {
  bool value = args[0];
  for (byte i = 1; i <= argCount - 1 ; i++)
    if (args[i] < 18){
      pinMode(args[i], value); 
  }

  result.singleByte = 2;
}

void sendIrValues() {
  result.singleByte = 0;

}

void switchFunction(byte *functionId) {
 
  switch (*functionId) {

    case 2: {  // set pinmode
      if (argCount > 1) 
        setPin();
    
      break;
    }

    case 3: {  // write pin
      if (argCount > 1) 
        writePin();
    
      break;
    }
      
    case 4: {  // read pin
      if (argCount > 0 && argCount < 9) 
        readPin();
     
      break;
    }  

    case 5: {  // read Ir values
      sendIrValues();
      break;
    }

    default:
      result.singleByte = 255;
      break;
  }
}

void requestEvent() {
  if (returnSingleByte) {
    Wire.write(result.singleByte);               // 1 byte
  } else {
    Wire.write((byte*)result.intArray, 16);      // 8 × int = 16 bytes
  }
}
