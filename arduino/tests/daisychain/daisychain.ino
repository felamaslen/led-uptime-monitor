int dataPin = 4;
int latchPin = 5;
int clockPin = 6;

byte eightOne = 0;
byte eightTwo = 0;

int numLeds = 15;

void setup() {
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  bitSet(eightOne, 1);
  
  updateShiftRegister();
}

int onLed = 0;

void loop() {
  /*
  if (onLed > 6) {
    bitClear(eightTwo, onLed - 7);
  }
  else {
    bitClear(eightOne, onLed);
  }
  
  onLed = (onLed + 1) % numLeds;
  
  if (onLed > 6) {
    bitSet(eightTwo, onLed - 7);
  }
  else {
    bitSet(eightOne, onLed);
  }
  
  updateShiftRegister();
  
  delay(500);
  // */
}

void updateShiftRegister() {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, eightOne);
  shiftOut(dataPin, clockPin, LSBFIRST, eightTwo);
  digitalWrite(latchPin, HIGH);
}
