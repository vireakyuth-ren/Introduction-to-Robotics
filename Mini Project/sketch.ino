#include <ESP32Servo.h>

#define TRIG_PIN 13
#define ECHO_PIN 39

String cmd = "";
int spd = 40;
int avoidspd = 45;
int searchspd = 42;

unsigned long lastDistTime = 0;
const unsigned long DIST_INTERVAL = 100;  // ms between distance reports

void setup() {
  Serial.begin(115200);

  pinMode(25, OUTPUT); pinMode(26, OUTPUT);
  ledcSetup(4, 1000, 8); ledcAttachPin(33, 4);

  pinMode(32, OUTPUT); pinMode(27, OUTPUT);
  ledcSetup(5, 1000, 8); ledcAttachPin(14, 5);

  pinMode(21, OUTPUT); pinMode(18, OUTPUT);
  ledcSetup(6, 1000, 8); ledcAttachPin(5, 6);

  pinMode(23, OUTPUT); pinMode(22, OUTPUT);
  ledcSetup(7, 1000, 8); ledcAttachPin(19, 7);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);  // 25ms timeout (~4m range)
  if (duration == 0) return -1;                     // no echo / out of range

  long distance = duration * 0.0343 / 2;
  return distance;
}

void moveForward() {
  ledcWrite(4, spd); digitalWrite(25, LOW); digitalWrite(26, HIGH);
  ledcWrite(5, spd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, spd); digitalWrite(21, LOW); digitalWrite(18, HIGH);
  ledcWrite(7, spd); digitalWrite(23, LOW); digitalWrite(22, HIGH);
}

void moveBackward() {
  ledcWrite(4, spd); digitalWrite(25, HIGH);  digitalWrite(26, LOW);
  ledcWrite(5, spd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, spd); digitalWrite(21, HIGH);  digitalWrite(18, LOW);
  ledcWrite(7, spd); digitalWrite(23, HIGH);  digitalWrite(22, LOW);
}

void moveRight() {
  ledcWrite(4, spd); digitalWrite(25, HIGH);  digitalWrite(26, LOW);
  ledcWrite(5, spd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, spd); digitalWrite(21, LOW); digitalWrite(18, HIGH);
  ledcWrite(7, spd); digitalWrite(23, LOW); digitalWrite(22, HIGH);
}

void moveLeft() {
  ledcWrite(4, spd); digitalWrite(25, LOW); digitalWrite(26, HIGH);
  ledcWrite(5, spd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, spd); digitalWrite(21, HIGH);  digitalWrite(18, LOW);
  ledcWrite(7, spd); digitalWrite(23, HIGH);  digitalWrite(22, LOW);
}

void moveSearchRight() {
  ledcWrite(4, searchspd); digitalWrite(25, HIGH);  digitalWrite(26, LOW);
  ledcWrite(5, searchspd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, searchspd); digitalWrite(21, LOW); digitalWrite(18, HIGH);
  ledcWrite(7, searchspd); digitalWrite(23, LOW); digitalWrite(22, HIGH);
}

void moveSearchLeft() {
  ledcWrite(4, searchspd); digitalWrite(25, LOW); digitalWrite(26, HIGH);
  ledcWrite(5, searchspd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, searchspd); digitalWrite(21, HIGH);  digitalWrite(18, LOW);
  ledcWrite(7, searchspd); digitalWrite(23, HIGH);  digitalWrite(22, LOW);
}

void avoidLeft() {
  ledcWrite(4, avoidspd); digitalWrite(25, LOW); digitalWrite(26, HIGH);
  ledcWrite(5, avoidspd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, avoidspd); digitalWrite(21, HIGH);  digitalWrite(18, LOW);
  ledcWrite(7, avoidspd); digitalWrite(23, HIGH);  digitalWrite(22, LOW);
}

void avoidRight() {
  ledcWrite(4, avoidspd); digitalWrite(25, HIGH);  digitalWrite(26, LOW);
  ledcWrite(5, avoidspd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, avoidspd); digitalWrite(21, LOW); digitalWrite(18, HIGH);
  ledcWrite(7, avoidspd); digitalWrite(23, LOW); digitalWrite(22, HIGH);
}

void stopMotors() {
  ledcWrite(4, 0); digitalWrite(25, LOW); digitalWrite(26, LOW);
  ledcWrite(5, 0); digitalWrite(32, LOW); digitalWrite(27, LOW);
  ledcWrite(6, 0); digitalWrite(21, LOW); digitalWrite(18, LOW);
  ledcWrite(7, 0); digitalWrite(23, LOW); digitalWrite(22, LOW);
}

void loop() {
  // --- handle incoming commands ---
  if (Serial.available()) {
    cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "Forward") {
      moveForward();
    } else if (cmd == "Backward") {
      moveBackward();
    } else if (cmd == "Left") {
      moveLeft();
    } else if (cmd == "Right") {
      moveRight();
    } else if (cmd == "AvoidLeft") {
      avoidLeft();
    } else if (cmd == "AvoidRight") {
      avoidRight();
    } else if (cmd == "SearchLeft") {
      moveSearchLeft();
    } else if (cmd == "SearchRight") {
      moveSearchRight();
    } else if (cmd == "Stop") {
      stopMotors();
    }
  }

  // --- periodically report distance ---
  unsigned long now = millis();
  if (now - lastDistTime >= DIST_INTERVAL) {
    lastDistTime = now;
    long dist = readDistanceCM();
    Serial.print("DIST:");
    Serial.println(dist);
  }
}
