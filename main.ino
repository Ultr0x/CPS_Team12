#include <Servo.h>
// first code to test the behaviour and connections
const int SERVO_SCAN_PIN = 11;  
const int SERVO_ACT_PIN  = 6;   
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

const float NEAR_CM = 25.0;   
const float FAR_CM  = 30.0;   

const int SCAN_MIN = 0;      
const int SCAN_MAX = 180;     
int scanAngle = 180;
int scanStep  = +1;           

const int ACT_MIN = 70;       
const int ACT_MAX = 110;      
int actAngle = 90;
int actStep  = +1;

enum Mode { SCANNING, ACTING };
Mode mode = SCANNING;

Servo servoScan;
Servo servoAct;

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (duration == 0) return 400.0; // timeout
  return duration / 58.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  servoScan.attach(SERVO_SCAN_PIN); // servo to cahnge the arm position
  servoAct.attach(SERVO_ACT_PIN); // servo to rotate the base
  servoScan.write(scanAngle);
  servoAct.write(actAngle);
}

void loop() {
  float distance = readDistanceCm();
  Serial.print("Distance (cm): ");
  Serial.println(distance);

  if (mode == SCANNING && distance <= NEAR_CM) {
    mode = ACTING;
  } else if (mode == ACTING && distance >= FAR_CM) {
    mode = SCANNING;
  }

  if (mode == SCANNING) {
    scanAngle += scanStep;
    if (scanAngle >= SCAN_MAX) { scanAngle = SCAN_MAX; scanStep = -scanStep; }
    if (scanAngle <= SCAN_MIN) { scanAngle = SCAN_MIN; scanStep = -scanStep; }
    servoScan.write(scanAngle);
  } else {
    actAngle += actStep;
    if (actAngle >= ACT_MAX) { actAngle = ACT_MAX; actStep = -actStep; }
    if (actAngle <= ACT_MIN) { actAngle = ACT_MIN; actStep = -actStep; }
    servoAct.write(actAngle);
  }

  delay(20);
}
