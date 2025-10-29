#include <Servo.h>
// first code to test the behaviour and connections with scaning udpate window 
const int SERVO_SCAN_PIN = 11;  
const int SERVO_ACT_PIN  = 6;   
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

const float NEAR_CM = 25.0;   
const float FAR_CM  = 30.0;   

const int SCAN_MIN = 0;      
const int SCAN_MAX = 180;     
const unsigned long SCAN_UPDATE_MS = 15; 
int scanAngle = 180;
int scanStep  = +1;           

const int ACT_MIN = 70;       
const int ACT_MAX = 110;      
const unsigned long ACT_UPDATE_MS = 25;
int actAngle = 90;
int actStep  = +1;

const unsigned long SENSOR_PERIOD_MS = 80;

enum Mode { SCANNING, ACTING };
Mode mode = SCANNING;

Servo servoScan;
Servo servoAct;

unsigned long tScan = 0, tAct = 0, tSensor = 0;

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long dur = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (dur == 0) return 400.0; // timeout = far distance
  return dur / 58.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  servoScan.attach(SERVO_SCAN_PIN);
  servoAct.attach(SERVO_ACT_PIN);
  servoScan.write(scanAngle);
  servoAct.write(actAngle);
}

void loop() {
  unsigned long now = millis();

  if (now - tSensor >= SENSOR_PERIOD_MS) {
    tSensor = now;
    float distance = readDistanceCm();
    Serial.print("Distance (cm): ");
    Serial.println(distance);

    if (mode == SCANNING && distance <= NEAR_CM) {
      mode = ACTING;
    } else if (mode == ACTING && distance >= FAR_CM) {
      mode = SCANNING;
    }
  }

  if (mode == SCANNING) {
    if (now - tScan >= SCAN_UPDATE_MS) {
      tScan = now;
      scanAngle += scanStep;
      if (scanAngle >= SCAN_MAX) { scanAngle = SCAN_MAX; scanStep = -scanStep; }
      if (scanAngle <= SCAN_MIN) { scanAngle = SCAN_MIN; scanStep = -scanStep; }
      servoScan.write(scanAngle);
    }
  } else {
    if (now - tAct >= ACT_UPDATE_MS) {
      tAct = now;
      actAngle += actStep;
      if (actAngle >= ACT_MAX) { actAngle = ACT_MAX; actStep = -actStep; }
      if (actAngle <= ACT_MIN) { actAngle = ACT_MIN; actStep = -actStep; }
      servoAct.write(actAngle);
    }
  }
}
