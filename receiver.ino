#include <WiFi.h>
#include <esp_now.h>

// Packet structure sent from HUZZAH
typedef struct {
  float distance;
  uint8_t mode;
} DataPacket;

DataPacket incoming;

// ---- NEW callback signature for ESP32-C6/ESP-IDF v5 ----
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&incoming, incomingData, sizeof(incoming));

  // Print sender MAC (optional)
  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.print("From ");
  Serial.print(macStr);
  Serial.print(" | Distance: ");
  Serial.print(incoming.distance, 1);
  Serial.print(" cm | Mode: ");
  Serial.println(incoming.mode == 0 ? "SCANNING" : "ACTING");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Turn Wi-Fi on in STA mode (needed for ESP-NOW and for MAC access)
  WiFi.mode(WIFI_STA);

  // ---- Print this device's Wi-Fi MAC address ----
  Serial.println();
  Serial.print("This ESP32-C6 Wi-Fi MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println();

  // ---- Initialize ESP-NOW ----
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  // Register the receive callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP32-C6 ready – waiting for packets...");
}

void loop() {}
