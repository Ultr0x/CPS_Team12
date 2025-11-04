#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// ---------------- Pins ----------------
const int SERVO_SCAN_PIN = 13; // continuous 360 servo (MG90D)
const int SERVO_ACT_PIN  = 14; // standard positional servo
const int TRIG_PIN = 27;
const int ECHO_PIN = 26;

// ---------------- Distances ----------------
// stop skanowania jeśli obiekt w odległości <= NEAR_CM
const float NEAR_CM = 20.0;   // zgodnie z Twoją prośbą: 20 cm
const float FAR_CM  = 30.0;   // wróć do skanowania gdy >= FAR_CM

// ---------------- Scan (continuous) servo speed config ----------------
// 0..180 mapping: 90 = stop, <90 = one direction, >90 = other direction
const int SCAN_STOP = 90;
const int SCAN_SPEED_FWD = 110; // drobna prędkość w jedną stronę
const int SCAN_SPEED_REV = 70;  // drobna prędkość w drugą stronę

// jak długo obracać w jednej stronie zanim zmieni kierunek (ms)
const unsigned long SCAN_DIR_DURATION_MS = 3000UL; // "few seconds" — dostosuj wg potrzeby

// ---------------- Acting servo (positional) ----------------
const int ACT_MIN = 70;
const int ACT_MAX = 110;
const unsigned long ACT_UPDATE_MS = 25;
int actAngle = 90;
int actStep = +1;

// ---------------- timing ----------------
const unsigned long SENSOR_PERIOD_MS = 80;
unsigned long tAct = 0, tSensor = 0;
unsigned long tScanDirToggle = 0;

// ---------------- modes ----------------
enum Mode { SCANNING, ACTING };
Mode mode = SCANNING;
Mode previousMode = SCANNING;

// ---------------- servos ----------------
Servo servoScan;
Servo servoAct;

// ---------------- ESP-NOW packet ----------------
typedef struct {
  float distance;
  uint8_t mode;
} DataPacket;
DataPacket packet;

// -------------  MAC of your ESP32-C6  -------------
uint8_t receiverAddress[] = {0x24, 0xEC, 0x4A, 0xCA, 0x60, 0x7C};

// ---- NEW callback signature for ESP-IDF 5.x ----
void OnSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long dur = pulseInLong(ECHO_PIN, HIGH, 30000UL);
  if (dur == 0) return 400.0;
  return dur / 58.0;
}

void setScanSpeedValue(int val) {
  // val: 0..180 mapping for continuous servo. 90 = stop.
  // Jeśli Twoja płytka/serwo lepiej reaguje na microseconds, zamień na writeMicroseconds().
  servoScan.write(constrain(val, 0, 180));
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  // register new-style send callback
  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, receiverAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  servoScan.attach(SERVO_SCAN_PIN);
  servoAct.attach(SERVO_ACT_PIN);

  // start: skanowanie w kierunku FWD
  tScanDirToggle = millis();
  setScanSpeedValue(SCAN_SPEED_FWD);

  // neutralne pozycje
  actAngle = 90;
  servoAct.write(actAngle);

  Serial.println("HUZZAH32 ready – sending to ESP32-C6.");
}

void loop() {
  unsigned long now = millis();
  static float distCm = 200.0;

  // --- sensor update + mode switching ---
  if (now - tSensor >= SENSOR_PERIOD_MS) {
    tSensor = now;
    distCm = readDistanceCm();

    // histerezę zachowujemy: przejście do ACTING gdy <= NEAR_CM, powrót do SCANNING gdy >= FAR_CM
    if (mode == SCANNING && distCm <= NEAR_CM) {
      mode = ACTING;
    } else if (mode == ACTING && distCm >= FAR_CM) {
      mode = SCANNING;
    }

    packet.distance = distCm;
    packet.mode = (uint8_t)mode;
    esp_now_send(receiverAddress, (uint8_t*)&packet, sizeof(packet));

    Serial.print("Distance: ");
    Serial.print(distCm, 1);
    Serial.print(" cm, Mode: ");
    Serial.println(mode == SCANNING ? "SCANNING" : "ACTING");
  }

  // --- mode transition handling (do once on change) ---
  if (mode != previousMode) {
    if (mode == ACTING) {
      // stop skanowania natychmiast
      setScanSpeedValue(SCAN_STOP);
      // ustaw act servo do oscylacji (zachowaj aktualne actAngle)
      tAct = now;
    } else { // mode == SCANNING
      // zatrzymaj act servo w pozycji neutralnej
      actAngle = 90;
      servoAct.write(actAngle);
      
      // wznow skanowanie w aktualnym kierunku (rozpocznij od FWD)
      tScanDirToggle = now;
      setScanSpeedValue(SCAN_SPEED_FWD);
    }
    previousMode = mode;
  }

  // --- SCANNING behaviour (continuous rotation) ---
  if (mode == SCANNING) {
    // co SCAN_DIR_DURATION_MS zmieniamy kierunek obrotu
    if (now - tScanDirToggle >= SCAN_DIR_DURATION_MS) {
      tScanDirToggle = now;
      // odczytaj aktualną prędkość i przełącz na przeciwną
      // Prosty sposób: sprawdź czy obecna wartość jest bliżej FWD czy REV
      // Nie mamy bezpośredniego odczytu; zapisujemy logikę zamiany:
      static bool lastWasFwd = true;
      lastWasFwd = !lastWasFwd;
      if (lastWasFwd) setScanSpeedValue(SCAN_SPEED_FWD);
      else setScanSpeedValue(SCAN_SPEED_REV);
    }
    // nic więcej — servo obraca się ciągle na zadanej prędkości
  } else { // ACTING behaviour
    // servoAct oscyluje pomiędzy ACT_MIN i ACT_MAX
    if (now - tAct >= ACT_UPDATE_MS) {
      tAct = now;
      actAngle += actStep;
      if (actAngle >= ACT_MAX) { actAngle = ACT_MAX; actStep = -1; }
      if (actAngle <= ACT_MIN) { actAngle = ACT_MIN; actStep = +1; }
      servoAct.write(actAngle);
    }
  }
}
