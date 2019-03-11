int dp1 = 4;
int lp1 = 5;
int cp1 = 6;

int dp2 = 8;
int lp2 = 9;
int cp2 = 10;

byte led[4] = {};

void updateShiftRegister() {
  digitalWrite(lp1, LOW);
  shiftOut(dp1, cp1, LSBFIRST, led[2]);
  shiftOut(dp1, cp1, LSBFIRST, led[3]);
  digitalWrite(lp1, HIGH);
  
  digitalWrite(lp2, LOW);
  shiftOut(dp2, cp2, LSBFIRST, led[0]);
  shiftOut(dp2, cp2, LSBFIRST, led[1]);
  digitalWrite(lp2, HIGH);
}

void toggleLed(int index, bool on) {
  int group = 0;
  int group_index = index;
  
  if (index > 6) {
    if (index > 14) {
      if (index > 21) {
        group = 3;
        group_index = index - 22;
      }
      else {
        group = 2;
        group_index = index - 15;
      }
    }
    else {
      group = 1;
      group_index = index - 7;
    }
  }

  if (on) {
    bitSet(led[group], group_index);
  }
  else {
    bitClear(led[group], group_index);
  }
}

void setup() {
  pinMode(dp1, OUTPUT);
  pinMode(dp2, OUTPUT);
  pinMode(lp1, OUTPUT);
  pinMode(lp2, OUTPUT);
  pinMode(cp1, OUTPUT);
  pinMode(cp2, OUTPUT);

  Serial.begin(9600);

  updateShiftRegister();
}

int ledIndex = 0;
int numLeds = 30;
bool sendingData = false;

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (!sendingData && c == 'b') { // b denotes "begin"
      ledIndex = 0;
      sendingData = true;

      // Serial.println("started sending data"); // debugging
    }
    else if (sendingData) {
      if (c == 'e') { // e denotes "end"
        sendingData = false;
      
        // Serial.println("ended sending data"); // debugging
      }
      else if (ledIndex < numLeds) { // ignore extra data
        toggleLed(ledIndex, c == '1' ? true : false);

        ledIndex++;
      }
    }
    else {
      // Serial.println("send a valid string please."); // debugging
    }
    
    updateShiftRegister();
  }
}
