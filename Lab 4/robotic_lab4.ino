#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#define TRIG_PIN 13
#define ECHO_PIN 39

Servo myServo;

int spd = 70;
long duration;
float distance;
int angle = 90;

const char* ssid     = "Robotic WIFI";
const char* password = "rbtWIFI@2025";

WebServer server(80);

// Mode flag
enum Mode { NONE, MANUAL, AUTOMATIC };
Mode currentMode = NONE;

// ─────────────────────────────────────────
// Motor functions
void moveForward() {
  ledcWrite(4, spd); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(5, spd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, spd); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(7, spd); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void moveBackward() {
  ledcWrite(4, spd); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(5, spd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, spd); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(7, spd); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void moveLeft() {
  ledcWrite(4, spd); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(5, spd); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(6, spd); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(7, spd); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void moveRight() {
  ledcWrite(4, spd); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(5, spd); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(6, spd); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(7, spd); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void stopMotors() {
  ledcWrite(4, 0); digitalWrite(25, LOW); digitalWrite(26, LOW);
  ledcWrite(5, 0); digitalWrite(32, LOW); digitalWrite(27, LOW);
  ledcWrite(6, 0); digitalWrite(21, LOW); digitalWrite(18, LOW);
  ledcWrite(7, 0); digitalWrite(23, LOW); digitalWrite(22, LOW);
}

// ─────────────────────────────────────────
// Manual mode — HTTP handlers
class ManualMode {
  public:
    static void handleForward() {
      if (currentMode != MANUAL) { server.send(403, "text/plain", "Not in manual mode"); return; }
      Serial.println("MANUAL: Forward");
      moveForward();
      server.send(200, "text/plain", "Forward");
    }

    static void handleBackward() {
      if (currentMode != MANUAL) { server.send(403, "text/plain", "Not in manual mode"); return; }
      Serial.println("MANUAL: Backward");
      moveBackward();
      server.send(200, "text/plain", "Backward");
    }

    static void handleLeft() {
      if (currentMode != MANUAL) { server.send(403, "text/plain", "Not in manual mode"); return; }
      Serial.println("MANUAL: Left");
      moveLeft();
      server.send(200, "text/plain", "Left");
    }

    static void handleRight() {
      if (currentMode != MANUAL) { server.send(403, "text/plain", "Not in manual mode"); return; }
      Serial.println("MANUAL: Right");
      moveRight();
      server.send(200, "text/plain", "Right");
    }

    static void handleStop() {
      if (currentMode != MANUAL) { server.send(403, "text/plain", "Not in manual mode"); return; }
      Serial.println("MANUAL: Stop");
      stopMotors();
      server.send(200, "text/plain", "Stop");
    }
};

// ─────────────────────────────────────────
// Automatic mode — obstacle avoidance
class AutoMode {
  private:
    static unsigned long avoidStart;
    static bool avoiding;

  public:
    static float readDistance() {
      digitalWrite(TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIG_PIN, LOW);
      long dur = pulseIn(ECHO_PIN, HIGH, 30000);
      if (dur == 0) return 999.0;
      return (dur * 0.0343) / 2.0;
    }

    static void run() {
      if (currentMode != AUTOMATIC) return;

      if (avoiding) {
        if (millis() - avoidStart >= 600) {
          avoiding = false;
          stopMotors();
        }
      } else {
        float dist = readDistance();
        Serial.print("AUTO distance: "); Serial.print(dist); Serial.println(" cm");

        if (dist < 30.0) {
          stopMotors();
          delay(50);
          moveLeft();
          avoidStart = millis();
          avoiding = true;
        } else {
          moveForward();
        }
      }
    }
};

unsigned long AutoMode::avoidStart = 0;
bool AutoMode::avoiding = false;

// ─────────────────────────────────────────
// HTTP route handlers
void handleMode() {
  if (server.hasArg("value")) {
    String m = server.arg("value");
    if (m == "manual") {
      currentMode = MANUAL;
      stopMotors();
      Serial.println("Mode: MANUAL");
      server.send(200, "text/plain", "Manual mode");
    } else if (m == "auto") {
      currentMode = AUTOMATIC;
      Serial.println("Mode: AUTOMATIC");
      server.send(200, "text/plain", "Auto mode");
    } else {
      server.send(400, "text/plain", "Unknown mode");
    }
  } else {
    server.send(400, "text/plain", "Missing value");
  }
}

void handleSpeed() {
  if (server.hasArg("value")) {
    spd = server.arg("value").toInt();
    Serial.print("Speed: "); Serial.println(spd);
  }
  server.send(200, "text/plain", "Speed set");
}

// Setup
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("ESP32 IP: "); Serial.println(WiFi.localIP());

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

  server.on("/forward",  ManualMode::handleForward);
  server.on("/backward", ManualMode::handleBackward);
  server.on("/left",     ManualMode::handleLeft);
  server.on("/right",    ManualMode::handleRight);
  server.on("/stop",     ManualMode::handleStop);
  server.on("/mode",     handleMode);
  server.on("/speed",    handleSpeed);

  server.begin();
}

// ─────────────────────────────────────────
//  Loop
void loop() {
  server.handleClient();
  AutoMode::run();
}