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

void resetClicksAndTimer(const unsigned long curr) {
  starboardClicks = 0;
  portClicks = 0;
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





























const unsigned char RPM_MESSAGE_ID = 0x10;
const int RPM_MESSAGE_PERIOD = 100;
const int RPM_READING_PERIOD = 100;
const unsigned int MOVING_AVG_WIDTH = RPM_MESSAGE_PERIOD / RPM_READING_PERIOD;
const int CLICKS_PER_REVOLUTION = 22;
const float REVOLUTIONS_PER_CLICK = 1.0 / CLICKS_PER_REVOLUTION;
const unsigned long MICROS_PER_MIN = 60000000;
unsigned int portRpms[MOVING_AVG_WIDTH];
unsigned int starboardRpms[MOVING_AVG_WIDTH];
unsigned int rpmIndex = 0;
unsigned long lastRpmTime = 0;


Task recordRpmTask(RPM_READING_PERIOD, recordRpm);
Task sendRpmCanMessageTask(RPM_MESSAGE_PERIOD, sendRpmCanMessage);

int truncateToByte(int val) {
  val = min(val, 255);
  val = max(val, 0);
  return val;
}

unsigned int toRpm(const unsigned long clicks, const unsigned long micros) {
  // IMPORTANT: don't change the order of these operations,
  // otherwise overflow might occur due to 32-bit resolution
  const float revs = clicks * REVOLUTIONS_PER_CLICK;
  const float revsPerMinute = (revs / micros) * MICROS_PER_MIN;
  return round(revsPerMinute);
}

void recordRpm(Task*) {
  return;
  // Cache values all at once for most accurate reading
  const unsigned long currTime = micros();
  const unsigned int cachedStarboardClicks = starboardClicks;
  const unsigned int cachedPortClicks = portClicks;

  // Once every ~70 minutes, micros() overflows back to zero.
  const bool timeOverflowed = currTime < lastRpmTime;

  // Go ahead and reset now so that interrupts can get back to work
  resetClicksAndTimer(currTime);

  if(timeOverflowed) {
    //Timer overflowed, do nothing this cycle
    return;
  }

  // Perform actual RPM calculations
  const unsigned long dt = currTime - lastRpmTime;
  const unsigned int starboardRpm = toRpm(cachedStarboardClicks, dt);
  const unsigned int portRpm = toRpm(cachedPortClicks, dt);

  // Record result, overwrite oldest existing record
  starboardRpms[rpmIndex] = starboardRpm;
  portRpms[rpmIndex] = portRpm;
  rpmIndex = (rpmIndex + 1) % MOVING_AVG_WIDTH;
}

void sendRpmCanMessage(Task*) {
  // Count total rpms
  unsigned long totStarboardRpm = 0;
  unsigned long totPortRpm = 0;
  for(unsigned int i = 0; i < MOVING_AVG_WIDTH; i++) {
    totStarboardRpm += starboardRpms[i];
    totPortRpm += portRpms[i];
  }

  // Average rpms
  unsigned int avgStarboardRpm = totStarboardRpm / MOVING_AVG_WIDTH;
  unsigned int avgPortRpm = totPortRpm / MOVING_AVG_WIDTH;

  (void)avgPortRpm;
  (void)avgStarboardRpm;
  // // Generate and send message
  // Frame rpmMessage = {
  //   .id=0x10,
  //   .body={
  //     highByte(avgStarboardRpm),
  //     lowByte(avgStarboardRpm),
  //     highByte(avgPortRpm),
  //     lowByte(avgPortRpm)
  //   },
  //   .len=4
  // };
  // // CAN().write(rpmMessage);
  // (void)rpmMessage;
}
