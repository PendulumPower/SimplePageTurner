#include <BleKeyboard.h>
#include <Adafruit_NeoPixel.h>

//Initialize BLE
BleKeyboard bleKeyboard("Page Turner", "Pendryx", 100);

//Define Pins
const int PIN_KEY_1 = D0; //Left
const int PIN_KEY_2 = D1; //Right
const int PIN_MULTI = D2; //Multi-purpose
const int PIN_NEOPIXEL = D10; //RGB LED

//Define Neopixel
Adafruit_NeoPixel strip(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

//Deine Keymaps
struct KeyProfile {
  const char* name;
  uint8_t key1;
  uint8_t key2;
  uint32_t ledColor;
};

KeyProfile profiles[] = {
  {"Left / Right Arrows", KEY_LEFT_ARROW, KEY_RIGHT_ARROW, strip.Color(0, 0, 255)},
  {"Up / Down Arrows", KEY_UP_ARROW, KEY_DOWN_ARROW, strip.Color(0, 255, 0)},
  {"Page Up / Down", KEY_PAGE_UP, KEY_PAGE_DOWN, strip.Color(128, 0, 128)},
  {"Space / Enter", 0x2C, KEY_RETURN, strip.Color(255, 128, 0)}
};
const int NUM_PROFILES = 4;
int currentProfile = 0;

//Timeout
const unsigned long SLEEP_TIMEOUT_MS = 5 * 60 * 1000; //5 min
unsigned long lastActivityTime = 0;
unsigned long multiButtonPressTime = 0;
bool multiButtonWasPressed = false;



void setup() {
  //Init Pins
  pinMode(PIN_KEY_1, INPUT_PULLUP);
  pinMode(PIN_KEY_2, INPUT_PULLUP);
  pinMode(PIN_MULTI, INPUT_PULLUP);

  //Init LED
  strip.begin();
  strip.setBrightness(40);
  strip.show();

  //Start BLE
  bleKeyboard.begin();

  //Config DS and Wake
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable((gpio_num_t)PIN_KEY_1, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_KEY_2, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_MULTI, GPIO_INTR_LOW_LEVEL);

  lastActivityTime = millis();
}
void loop() {
  unsigned long currentMillis = millis();

  if (bleKeyboard.isConnected()) {
    //Show profile color
    strip.setPixelColor(0, profiles[currentProfile].ledColor);
    strip.show();

    //Page Turning
    if (digitalRead(PIN_KEY_1) == LOW) {
      delay(20);
      if (digitalRead(PIN_KEY_1) == LOW) {
        bleKeyboard.write(profiles[currentProfile].key1);
        lastActivityTime = currentMillis;
        while (digitalRead(PIN_KEY_1) == LOW); //Wait until released
      }
    }

    if (digitalRead(PIN_KEY_2) == LOW) {
      delay(20);
      if (digitalRead(PIN_KEY_1) == LOW) {
        bleKeyboard.write(profiles[currentProfile].key2);
        lastActivityTime = currentMillis;
        while (digitalRead(PIN_KEY_2) == LOW); //Wait until released
      }
    }

    if (digitalRead(PIN_MULTI) == LOW) {
      if (!multiButtonWasPressed) {
        multiButtonPressTime = currentMillis;
        multiButtonWasPressed = true;
      }
    } else {
      if (multiButtonWasPressed) {
        unsigned long pressDuration = currentMillis - multiButtonPressTime;
        lastActivityTime = currentMillis;

        if (pressDuration > 3000) { //Long press
        flashLED(strip.Color(255, 255, 255), 5, 150);
      } else if (pressDuration > 50) { //Short press
        currentProfile = (currentProfile + 1) % NUM_PROFILES;

        flashLED(profiles[currentProfile].ledColor, 2, 150);
      }
      multiButtonWasPressed = false;
      }
  }

  //DS Clock
  if (currentMillis - lastActivityTime > SLEEP_TIMEOUT_MS) {
    goToDeepSleep();
  }

} else {
  //Not Connected
  static unsigned long lastPulse = 0;
  static int pulseBrightness = 0;
  static int fadeDelta = 5;

  if (currentMillis - lastPulse > 30) {
    pulseBrightness += fadeDelta;
    if (pulseBrightness <= 0 || pulseBrightness >= 100) {
      fadeDelta = -fadeDelta;
    }
    strip.setBrightness(abs(pulseBrightness));
    strip.setPixelColor(0, strip.Color(0, 255, 255));
    strip.show();
    lastPulse = currentMillis;
  }
}

delay(10);
}

void flashLED(uint32_t color, int times, int delayMs) {
  strip.setBrightness(50);
  for (int i = 0; i < times; i++) {
    strip.setPixelColor(0, color);
    strip.show();
    delay(delayMs);
    strip.setPixelColor(0, 0);
    strip.show();
    delay(delayMs);
  }
}

void goToDeepSleep() {
  bleKeyboard.releaseAll();
  strip.setPixelColor(0, 0);
  strip.show();
  esp_deep_sleep_start();
}