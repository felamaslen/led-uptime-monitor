int latchPin = 5;
int clockPin = 6;
int dataPin = 4;

byte leds = 0;

// this is for a separate indicator / debug LED
int ledPin = 13;

int ledIndex = 0;

void updateShiftRegister() {
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, LSBFIRST, leds);
  digitalWrite(latchPin, 1);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 1);

  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '1') {
      bitSet(leds, ledIndex);
    }
    else {
      bitClear(leds, ledIndex);
    }

    ledIndex = (ledIndex + 1) % 8;
  
    updateShiftRegister();
  }
}
