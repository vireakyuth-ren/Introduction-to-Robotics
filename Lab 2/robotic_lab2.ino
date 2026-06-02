#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

const uint16_t RECV_PIN = 36;
IRrecv irrecv(RECV_PIN);
decode_results results;

int speed = 50;
String digitBuffer = "";

void setup() {
  pinMode(25, OUTPUT); pinMode(26, OUTPUT);
  ledcSetup(0, 20000, 8); ledcAttachPin(33, 0);

  pinMode(32, OUTPUT); pinMode(27, OUTPUT);
  ledcSetup(1, 20000, 8); ledcAttachPin(14, 1);

  pinMode(21, OUTPUT); pinMode(18, OUTPUT);
  ledcSetup(2, 20000, 8); ledcAttachPin(5, 2);

  pinMode(23, OUTPUT); pinMode(22, OUTPUT);
  ledcSetup(3, 20000, 8); ledcAttachPin(19, 3);

  Serial.begin(115200);
  irrecv.enableIRIn();
}

void forward() {
  ledcWrite(0, speed); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(1, speed); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(2, speed); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(3, speed); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void backward() {
  ledcWrite(0, speed); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(1, speed); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(2, speed); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(3, speed); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void left() {
  ledcWrite(0, speed); digitalWrite(25, LOW);  digitalWrite(26, HIGH);
  ledcWrite(1, speed); digitalWrite(32, HIGH); digitalWrite(27, LOW);
  ledcWrite(2, speed); digitalWrite(21, HIGH); digitalWrite(18, LOW);
  ledcWrite(3, speed); digitalWrite(23, HIGH); digitalWrite(22, LOW);
}

void right() {
  ledcWrite(0, speed); digitalWrite(25, HIGH); digitalWrite(26, LOW);
  ledcWrite(1, speed); digitalWrite(32, LOW);  digitalWrite(27, HIGH);
  ledcWrite(2, speed); digitalWrite(21, LOW);  digitalWrite(18, HIGH);
  ledcWrite(3, speed); digitalWrite(23, LOW);  digitalWrite(22, HIGH);
}

void stopMotors() {
  ledcWrite(0, 0); digitalWrite(25, LOW); digitalWrite(26, LOW);
  ledcWrite(1, 0); digitalWrite(32, LOW); digitalWrite(27, LOW);
  ledcWrite(2, 0); digitalWrite(21, LOW); digitalWrite(18, LOW);
  ledcWrite(3, 0); digitalWrite(23, LOW); digitalWrite(22, LOW);
}

void loop() {
  if (irrecv.decode(&results)) {
    uint32_t irCode = results.value;
    Serial.print("IR: 0x"); Serial.println(irCode, HEX);

    switch (irCode) {

      // Motion
      case 0xFF18E7: forward();    break;
      case 0xFF4AB5: backward();   break;
      case 0xFF10EF: left();       break;
      case 0xFF5AA5: right();      break;
      case 0xFF38C7: stopMotors(); break;

      // Speed adjust
      case 0xFF6897:  // '*' decrease
        speed -= 5;
        speed = constrain(speed, 0, 100);
        Serial.print("Speed: "); Serial.println(speed);
        break;

      case 0xFFB04F:  // '#' increase
        speed += 5;
        speed = constrain(speed, 0, 100);
        Serial.print("Speed: "); Serial.println(speed);
        break;

      // Digit input (1–9 builds buffer)
      case 0xFFA25D: digitBuffer += "1"; break;
      case 0xFF629D: digitBuffer += "2"; break;
      case 0xFFE21D: digitBuffer += "3"; break;
      case 0xFF22DD: digitBuffer += "4"; break;
      case 0xFF02FD: digitBuffer += "5"; break;
      case 0xFFC23D: digitBuffer += "6"; break;
      case 0xFFE01F: digitBuffer += "7"; break;
      case 0xFFA857: digitBuffer += "8"; break;
      case 0xFF906F: digitBuffer += "9"; break;

      // Button 0: confirm numeric speed
      case 0xFF9867:
        if (digitBuffer.length() > 0) {
          speed = constrain(digitBuffer.toInt(), 0, 100);
          Serial.print("Speed set to: "); Serial.println(speed);
          digitBuffer = "";
        }
        break;
    }

    irrecv.resume();
  }
}