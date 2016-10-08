#include <Arduino.h>

#include "Wheel_Speed_Test.h"

// Pins 2 and 3 are INT0 and INT1 (respectively) for 328p
// Pins 21 and 20 are INT0 and INT1 (respectively) for 2560
const int STARBOARD_ENCODER_PIN = 2;
const int PORT_ENCODER_PIN = 3;

volatile unsigned int starboardClicks = 0;
volatile unsigned int portClicks = 0;

void logStarboardEncoderClick() {
  starboardClicks++;
  Serial.print("Right click ");
  Serial.println(starboardClicks);
}

void logPortEncoderClick() {
  portClicks++;
  Serial.print("Left click ");
  Serial.println(portClicks);
}

// Implementation
void setup() {
  Serial.begin(115200);

  pinMode(PORT_ENCODER_PIN, INPUT_PULLUP);
  pinMode(STARBOARD_ENCODER_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PORT_ENCODER_PIN), logPortEncoderClick, CHANGE);
  attachInterrupt(digitalPinToInterrupt(STARBOARD_ENCODER_PIN), logStarboardEncoderClick, CHANGE);

  Serial.println("Started Can Node");
}
