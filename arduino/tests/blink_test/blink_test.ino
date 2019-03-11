/*
Adafruit Arduino - Lesson 4. 8 LEDs and a Shift Register
*/
 
int dataPin = 4;
int latchPin = 5;
int clockPin = 6;
 
byte leds = 0;
byte leds2 = 0;
 
void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}
 
void loop() {
  leds = 0;
  leds2 = 0;
  updateShiftRegister();
  delay(1000);
  for (int i = 0; i < 8; i++) {
    //bitClear(leds, (i + 7) % 8);
    bitSet(leds, i);
    bitSet(leds2, i);
    updateShiftRegister();
    delay(1000);
  }
}
 
void updateShiftRegister() {
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, leds);
   shiftOut(dataPin, clockPin, LSBFIRST, leds2);
   digitalWrite(latchPin, HIGH);
}
