#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#define TRIG_PIN 13
#define ECHO_PIN 39

#include <DabbleESP32.h>
#include <ESP32Servo.h>

const char* BT_NAME = "swag robot";

Servo myServo;

int speed = 70;
long duration;
float distance;
int angle = 90;


// Replace your debounce flags with these
unsigned long lastSquareTime = 0;
unsigned long lastCircleTime = 0;
const unsigned long DEBOUNCE_MS = 300; // adjust if needed

// 0 = idle, 1 = manual, 2 = automatic
int mode = 0;

// ── Motor setup ───────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Dabble.begin(BT_NAME);

  pinMode(25, OUTPUT); pinMode(26, OUTPUT);
  ledcSetup(4, 20000, 8); ledcAttachPin(33, 4);

  pinMode(32, OUTPUT); pinMode(27, OUTPUT);
  ledcSetup(5, 20000, 8); ledcAttachPin(14, 5);

  pinMode(21, OUTPUT); pinMode(18, OUTPUT);
  ledcSetup(6, 20000, 8); ledcAttachPin(5, 6);

  pinMode(23, OUTPUT); pinMode(22, OUTPUT);
  ledcSetup(7, 20000, 8); ledcAttachPin(19, 7);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myServo.attach(17);
  myServo.write(angle);
  stopMotors();
}

// ── Drive functions ───────────────────────────────────────────────────────────
void forward() {
  ledcWrite(4, speed); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(5, speed); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, speed); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(7, speed); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void backward() {
  ledcWrite(4, speed); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(5, speed); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, speed); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(7, speed); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void left() {
  ledcWrite(4, speed); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(5, speed); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, speed); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(7, speed); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void right() {
  ledcWrite(4, speed); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(5, speed); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, speed); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(7, speed); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void stopMotors() {
  ledcWrite(4, 0); digitalWrite(25, LOW); digitalWrite(26, LOW);
  ledcWrite(5, 0); digitalWrite(32, LOW); digitalWrite(27, LOW);
  ledcWrite(6, 0); digitalWrite(21, LOW); digitalWrite(18, LOW);
  ledcWrite(7, 0); digitalWrite(23, LOW); digitalWrite(22, LOW);
}

// ── Ultrasonic helper ─────────────────────────────────────────────────────────
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2.0;
}

// ── Timing variables for non-blocking auto mode ───────────────────────────────
unsigned long avoidStart = 0;
bool avoiding = false;

// ── Manual mode ───────────────────────────────────────────────────────────────
void manual() {
  if      (GamePad.isUpPressed())    forward();
  else if (GamePad.isDownPressed())  backward();
  else if (GamePad.isLeftPressed())  left();
  else if (GamePad.isRightPressed()) right();
  else                               stopMotors();

  unsigned long now = millis();

  if (GamePad.isSquarePressed() && (now - lastSquareTime >= DEBOUNCE_MS)) {
    lastSquareTime = now;
    angle += 10;
    angle = constrain(angle, 80, 120);
    myServo.write(angle);
    Serial.print("Angle: "); Serial.println(angle); // debug
  }
  if (GamePad.isCirclePressed() && (now - lastCircleTime >= DEBOUNCE_MS)) {
    lastCircleTime = now;
    angle -= 10;
    angle = constrain(angle, 80, 120);
    myServo.write(angle);
    Serial.print("Angle: "); Serial.println(angle); // debug
  }
  if (GamePad.isCrossPressed()) {
    angle = 80;
    myServo.write(angle);
  }
  if (GamePad.isTrianglePressed()) {
    angle = 120;
    myServo.write(angle);
  }
}

// ── Automatic mode (non-blocking) ─────────────────────────────────────────────
void automatic() {
  if (avoiding) {
    // Currently turning — wait 600 ms then stop and resume forward
    if (millis() - avoidStart >= 600) {
      avoiding = false;
      stopMotors();
    }
    // else: keep turning (left() was already called)
  } else {
    distance = readDistance();
    Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");

    if (distance < 30.0) {
      stopMotors();
      delay(50);           // tiny pause — safe, very short
      left();
      avoidStart = millis();
      avoiding = true;
    } else {
      forward();
    }
  }
}

// ── Main loop ─────────────────────────────────────────────────────────────────
void loop() {
  Dabble.processInput();

  // Latch mode on button press
  if (GamePad.isStartPressed())  { mode = 2; Serial.println("Mode: AUTO"); }
  if (GamePad.isSelectPressed()) { mode = 1; Serial.println("Mode: MANUAL"); }

  switch (mode) {
    case 1: manual();    break;
    case 2: automatic(); break;
    default: stopMotors(); break;
  }
}
