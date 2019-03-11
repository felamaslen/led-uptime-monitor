/**
 * leds.ino
 * Written by Fela Maslen, 2016
 */

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define WORD_ACED 'A'
#define WORD_BEEF 'B'
#define WORD_BABE 'C'
#define WORD_DEAD 'D'
#define WORD_DEAF 'F'

#define DISPLAY_BRIGHTNESS 2
#define CMD_QUIET 'Q'

Adafruit_7segment display = Adafruit_7segment();

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

  display.begin(0x70);
  display.setBrightness(DISPLAY_BRIGHTNESS); // 0 to 15
  display.drawColon(true);
  display.writeDisplay();

  updateShiftRegister();
}

int displayIndex = 0;
bool sendWord = true;
bool decimalPoint = false;

int ledIndex = 0;
int numLeds = 30;

int serialInt;

int sendingData = 0;

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    int brightness = DISPLAY_BRIGHTNESS;

    if (sendingData == 0) {
      if (c == 'c') { // begin display
        displayIndex = 0;
        sendingData = 2;
        sendWord = true;
      }
      else if (c == 'b') { // begin LEDs
        ledIndex = 0;
        sendingData = 1;
      }
    }
    else {
      switch (sendingData) {
      case 2:

        switch (c) {
        case WORD_ACED:
          display.print(0xACED, HEX);
          break;
        case WORD_BEEF:
          display.print(0xBEEF, HEX);
          break;
        case WORD_BABE:
          display.print(0xBABE, HEX);
          break;
        case WORD_DEAD:
          display.print(0xDEAD, HEX);
          break;
        case WORD_DEAF:
          display.print(0xDEAF, HEX);
          break;
        case CMD_QUIET:
          brightness = 0;
          break;
        default:
          sendWord = false;

          serialInt = c - '0';

          if (displayIndex == 4) {
            // colon
            display.drawColon(serialInt == 1);
          }
          else if (displayIndex % 2 == 0) {
            // decimal point switcher
            decimalPoint = serialInt == 1 ? true : false;
          }
          else if (displayIndex != 5) {
            int displayIndexRaw = (displayIndex - 1) / 2;

            display.writeDigitNum(
              displayIndexRaw,
              serialInt,
              decimalPoint
            );
          }
          
          displayIndex++;

          if (displayIndex > 9) {
            sendingData = 0;

            display.writeDisplay();
          }
        }

        display.setBrightness(brightness);

        if (sendWord) {
          sendingData = 0;
          display.writeDisplay();
        }

        break;

      case 1:
      default:
        if (c == 'e') { // end
          sendingData = 0;
        }
        else if (ledIndex < numLeds) { // ignore extra data
          toggleLed(ledIndex, c == '1' ? true : false);

          ledIndex++;
        }
      }
    }
    
    updateShiftRegister();
  }
}
