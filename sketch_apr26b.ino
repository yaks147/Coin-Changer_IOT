#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== SERVOS =====
#define SERVO1_PIN 2
#define SERVO5_PIN 3
#define SERVO10_PIN 5

Servo servo1;
Servo servo5;
Servo servo10;

// ===== JOYSTICK =====
#define VRX_PIN 0
#define SW_PIN 6

// ===== BILL ACCEPTOR =====
#define BILL_PIN 4

volatile int pulseCount = 0;
volatile unsigned long lastPulseTime = 0;

int credit = 0;

// ===== MENU =====
int menuIndex = 0;
String menu[3] = {"1 PESO", "5 PESO", "10 PESO"};
int values[3] = {1, 5, 10};

// ===== BUTTON CONTROL =====
bool buttonState = false;
bool longPress = false;
unsigned long pressStart = 0;

// ================= BILL INTERRUPT =================
void IRAM_ATTR billPulse() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = millis();

  if (now - lastInterrupt > 70) {
    pulseCount++;
    lastPulseTime = now;
  }

  lastInterrupt = now;
}

// ================= BILL PROCESS =================
void processBill(int pulses) {

  Serial.print("Pulse: ");
  Serial.println(pulses);

  if (pulses >= 1 && pulses <= 4) credit += 20;
  else if (pulses >= 5 && pulses <= 10) credit += 50;
  else if (pulses >= 11 && pulses <= 15) credit += 100;

  Serial.print("Credit: ");
  Serial.println(credit);
}

// ================= SERVO CONTROL =================
void releaseCoin(int pin) {

  Servo s;
  s.attach(pin);

  s.write(90);
  delay(250);

  s.write(0);
  delay(250);

  s.detach();
}

// ================= GET SERVO =================
int getServo(int index) {
  if (index == 0) return SERVO1_PIN;
  if (index == 1) return SERVO5_PIN;
  return SERVO10_PIN;
}

// ================= SINGLE DISPENSE =================
void singleDispense(int index) {

  int value = values[index];

  if (credit < value) {
    Serial.println("NO CREDIT");
    return;
  }

  releaseCoin(getServo(index));
  credit -= value;

  Serial.println("SINGLE DISPENSE DONE");
}

// ================= AUTO DISPENSE =================
void autoDispense(int index) {

  int value = values[index];

  Serial.println("AUTO MODE START");

  while (credit >= value) {

    releaseCoin(getServo(index));
    credit -= value;

    showUI("AUTO DISPENSING");
    delay(300);
  }

  Serial.println("AUTO DONE");
}

// ================= JOYSTICK CONTROL =================
void readJoystick() {

  int x = analogRead(VRX_PIN);

  if (x < 1000) {
    menuIndex--;
    if (menuIndex < 0) menuIndex = 2;
    delay(200);
  }

  if (x > 3000) {
    menuIndex++;
    if (menuIndex > 2) menuIndex = 0;
    delay(200);
  }

  int btn = digitalRead(SW_PIN);

  // PRESS START
  if (btn == LOW && !buttonState) {
    buttonState = true;
    pressStart = millis();
  }

  // HOLD
  if (btn == LOW && buttonState) {
    if (millis() - pressStart > 800) {
      longPress = true;
    }
  }

  // RELEASE
  if (btn == HIGH && buttonState) {

    buttonState = false;

    if (!longPress) {
      singleDispense(menuIndex);
    } else {
      autoDispense(menuIndex);
    }

    longPress = false;
  }
}

// ================= OLED =================
void showUI(String status) {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println("ARCADE VENDING PRO");

  display.setCursor(0, 12);
  display.print("CREDIT: ");
  display.println(credit);

  for (int i = 0; i < 3; i++) {

    display.setCursor(0, 28 + (i * 10));

    if (i == menuIndex) display.print("> ");
    else display.print("  ");

    display.println(menu[i]);
  }

  display.setCursor(0, 58);
  display.println(status);

  display.display();
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(BILL_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BILL_PIN), billPulse, FALLING);

  showUI("READY");

  Serial.println("PRO ARCADE SYSTEM READY");
}

// ================= LOOP =================
void loop() {

  // BILL PROCESS
  if (pulseCount > 0 && millis() - lastPulseTime > 700) {
    processBill(pulseCount);
    pulseCount = 0;
  }

  // JOYSTICK CONTROL
  readJoystick();

  // UI UPDATE
  showUI("WAITING");

  delay(100);
} fix the servo not moving past