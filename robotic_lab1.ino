const int joyX = 34;
const int joyY = 35;

int xValue = 0;
int yValue = 0;

// Define deadzone thresholds around the center (2048)
const int CENTER_VAL = 2048;
const int DEADZONE = 200; 
const int MIN_SPEED = 0;
const int MAX_SPEED = 100;
int forward_speed = 50, rotation_speed = 50;  

int percentToPWM(int percent) {
  percent = constrain(percent, 0, 100);
  return map(percent, 0, 100, 0, 255);
} 

void setup() {

  pinMode(16, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  Serial.begin(115200);

  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  ledcSetup(0, 20000, 8);
  ledcAttachPin(33, 0);

  pinMode(32, OUTPUT);
  pinMode(27, OUTPUT);
  ledcSetup(1, 20000, 8);
  ledcAttachPin(14, 1);

  pinMode(21, OUTPUT);
  pinMode(18, OUTPUT);
  ledcSetup(2, 20000, 8);
  ledcAttachPin(5, 2);
  
  pinMode(23, OUTPUT);
  pinMode(22, OUTPUT);
  ledcSetup(3, 20000, 8);
  ledcAttachPin(19, 3);

  Serial.println("ESP32 Joystick Reading Started");
}

void forward(int forward_speed) {
  int pwm = percentToPWM(forward_speed);

  ledcWrite(0, pwm);
  digitalWrite(25, HIGH);
  digitalWrite(26, LOW);

  ledcWrite(1, pwm);
  digitalWrite(32, LOW);
  digitalWrite(27, HIGH);
  
  ledcWrite(2, pwm);
  digitalWrite(21, HIGH);
  digitalWrite(18, LOW);
  
  ledcWrite(3, pwm);
  digitalWrite(23, HIGH);
  digitalWrite(22, LOW);
}

void backward(int forward_speed) {
  int pwm = percentToPWM(forward_speed);

  ledcWrite(0, pwm);
  digitalWrite(25, LOW);
  digitalWrite(26, HIGH);

  ledcWrite(1, pwm);
  digitalWrite(32, HIGH);
  digitalWrite(27, LOW);
  
  ledcWrite(2, pwm);
  digitalWrite(21, LOW);
  digitalWrite(18, HIGH);
  
  ledcWrite(3, pwm);
  digitalWrite(23, LOW);
  digitalWrite(22, HIGH);
}

void left(int rotation_speed) {
  int pwm = percentToPWM(rotation_speed);

  ledcWrite(0, pwm);
  digitalWrite(25, LOW);
  digitalWrite(26, HIGH);

  ledcWrite(1, pwm);
  digitalWrite(32, HIGH);
  digitalWrite(27, LOW);
  
  ledcWrite(2, pwm);
  digitalWrite(21, HIGH);
  digitalWrite(18, LOW);
  
  ledcWrite(3, pwm);
  digitalWrite(23, HIGH);
  digitalWrite(22, LOW);
}

void right(int rotation_speed) {
  int pwm = percentToPWM(rotation_speed);

  ledcWrite(0, pwm);
  digitalWrite(25, HIGH);
  digitalWrite(26, LOW);

  ledcWrite(1, pwm);
  digitalWrite(32, LOW);
  digitalWrite(27, HIGH);
  
  ledcWrite(2, pwm);
  digitalWrite(21, LOW);
  digitalWrite(18, HIGH);
  
  ledcWrite(3, pwm);
  digitalWrite(23, LOW);
  digitalWrite(22, HIGH);
}

void stop() {
  ledcWrite(0, 0);
  digitalWrite(25, LOW);
  digitalWrite(26, LOW);

  ledcWrite(1, 0);
  digitalWrite(32, LOW);
  digitalWrite(27, LOW);
  
  ledcWrite(2, 0);
  digitalWrite(21, LOW);
  digitalWrite(18, LOW);
  
  ledcWrite(3, 0);
  digitalWrite(23, LOW);
  digitalWrite(22, LOW);
}



void loop() {
  int button_up    = digitalRead(16);
  int button_down  = digitalRead(15);
  int button_left  = digitalRead(2);
  int button_right = digitalRead(4);

  if (button_up == LOW) {
    forward_speed += 5;
    delay(200);
  }
  else if (button_down == LOW) {
    forward_speed -= 5;
    delay(200);
  }
  else if (button_left == LOW) {
    rotation_speed += 5;
    delay(200);
  }
  else if (button_right == LOW) {
    rotation_speed -= 5;
    delay(200);
}

forward_speed = constrain(forward_speed, 0, 100);
rotation_speed = constrain(rotation_speed, 0, 100);

  xValue = analogRead(joyX);
  yValue = analogRead(joyY);

  Serial.print("X: "); Serial.print(xValue);
  Serial.print(" | Y: "); Serial.println(yValue);
  Serial.print(" | Forward Speed: "); Serial.print(forward_speed);
  Serial.print("% | Rotation Speed: "); Serial.print(rotation_speed);
  Serial.println("%");

  // Center around 0
  int x = xValue - CENTER_VAL;
  int y = yValue - CENTER_VAL;

  // Apply deadzone
  if (abs(x) < DEADZONE) x = 0;
  if (abs(y) < DEADZONE) y = 0;

  // Speed ceiling in percentage 0-100
  int fPWM_max = forward_speed;
  int rPWM_max = rotation_speed;

  // Scale linearly with joystick displacement
  int fPWM = map(abs(y), DEADZONE, 2048, 0, fPWM_max);
  int rPWM = map(abs(x), DEADZONE, 2048, 0, rPWM_max);

  fPWM = constrain(fPWM, 0, 100);
  rPWM = constrain(rPWM, 0, 100);

    // Movement
  if (x == 0 && y == 0) {
    stop();
  } else if (abs(y) >= abs(x)) {
    if (y < 0) forward(fPWM);
    else        backward(fPWM);
  } else {
    if (x < 0) right(rPWM);
    else        left(rPWM);
  }

  delay(50);
}
