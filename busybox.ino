#include <STM32LowPower.h>
#include <STM32RTC.h>

STM32RTC& rtc = STM32RTC::getInstance();

#define NUM_PINS 5
#define LIGHT_TIMEOUT 10

// Button Order: Red, Yellow, Green, Blue, White
const int buttons[NUM_PINS] = { PB15, PB14, PB12, PB11, PB13 };
const int leds[NUM_PINS] = { PA7, PA6, PA9, PA8, PB0 };
uint32_t timeout[NUM_PINS];
bool lastState[NUM_PINS];

void setup() {
  LowPower.begin();
  rtc.begin();

  // set an arbitrary epoch time, we just want to know how much time has passed
  rtc.setEpoch(10);

  // setup all the buttons and LEDs
  for(int i = 0; i < NUM_PINS; i++) {
    pinMode(buttons[i], INPUT_PULLDOWN);
    pinMode(leds[i], OUTPUT);
    LowPower.attachInterruptWakeup(buttons[i], buttonPushed, RISING, DEEP_SLEEP_MODE);
    digitalWrite(leds[i], LOW);
    timeout[i] = 0;
    lastState[i] = false;
  }

  // slightly less power usage as input
  pinMode(LED_BUILTIN, INPUT);

  // wake up less often to save power
  HAL_SetTickFreq(HAL_TICK_FREQ_10HZ);
}

bool pollButton(uint32_t now, int i) {
  bool pushed = digitalRead(buttons[i]);

  if(pushed && !lastState[i]) {
    if(timeout[i]) {
      digitalWrite(leds[i], LOW);
      timeout[i] = 0;
    } else {
      digitalWrite(leds[i], HIGH);
      timeout[i] = now + LIGHT_TIMEOUT;
    }
  }

  lastState[i] = pushed;

  if(timeout[i] && now > timeout[i]) {
    digitalWrite(leds[i], LOW);
    timeout[i] = 0;
  }

  return pushed || timeout[i] > 0;
}

void loop() {
  uint32_t now = rtc.getEpoch();
  bool quickSleep = false;

  for(int i = 0; i < NUM_PINS; i++) {
    quickSleep |= pollButton(now, i);
  }

  // do we need to wake back up and check timeouts?
  if(quickSleep) {
    // wakes up on next systick, about 100ms
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  } else {
    LowPower.deepSleep();
  }
}

void buttonPushed() {
  // deepSleep is slow to return with HAL_TICK_FREQ_10HZ enabled, so poll everything here right away
  uint32_t now = rtc.getEpoch();
  for(int i = 0; i < NUM_PINS; i++) {
    pollButton(now, i);
  }
}
