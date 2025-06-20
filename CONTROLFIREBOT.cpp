
#include <esp_now.h>
#include <WiFi.h>
// Chân kết nối joystick, nút và chế độ
#define VRX     34
#define VRY     33
#define SW      5
#define BTN_FWD 32
#define BTN_REV 27
#define BTN_LEF 26
#define BTN_RIT 35
#define BTN_MODE 25  // Nút chuyển chế độ
// Chân encoder
#define ENCODER_DT  13
#define ENCODER_CLK 14
// Địa chỉ MAC ESP32 nhận
uint8_t broadcastAddress[] = {0x04, 0x83, 0x08, 0x58, 0x9A, 0xA8};
// Cấu trúc dữ liệu gửi đi
typedef struct struct_message {
  int x;
  int y;
  int swi;
  int front;
  int back;
  int left;
  int right;
  int speed;
  int mode;  // Chế độ điều khiển (0: PC, 1: Tay điều khiển)
} struct_message;
struct_message data;
esp_now_peer_info_t peerInfo;
// Biến encoder và chế độ
int encoderPos = 175;
int lastCLKState = 0;
int lastModeButtonState = HIGH;  // Trạng thái của nút BTN_MODE từ lần trước
int modeState = 0;               // Trạng thái hiện tại của chế độ
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Data send failed");
  }
}
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Add peer failed");
    return;
  }
  // Cấu hình input
  pinMode(VRX, INPUT);
  pinMode(VRY, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(BTN_FWD, INPUT);
  pinMode(BTN_REV, INPUT);
  pinMode(BTN_LEF, INPUT);
  pinMode(BTN_RIT, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_CLK, INPUT);
  pinMode(BTN_MODE, INPUT);  // Nút chuyển chế độ
  lastCLKState = digitalRead(ENCODER_CLK);
}
void loop() {
  // Đọc encoder
  int currentCLKState = digitalRead(ENCODER_CLK);
  if (currentCLKState != lastCLKState) {
    if (digitalRead(ENCODER_DT) != currentCLKState) {
      encoderPos+=3;
    } else {
      encoderPos-=3;
    }
    encoderPos = constrain(encoderPos, 0, 255);
  }
  lastCLKState = currentCLKState;
  // doc nut mode
  // Đọc trạng thái nút chế độ
  int currentModeButtonState = digitalRead(BTN_MODE);
  // Kiểm tra nếu nút được nhấn và trước đó không nhấn
  if (currentModeButtonState == LOW && lastModeButtonState == HIGH) {
    // Nếu nút vừa được nhấn, thay đổi chế độ
    modeState = (modeState == 1) ? 0 : 1;  // Chuyển đổi giữa 0 và 1
    Serial.print("Mode: ");
    Serial.println(modeState);
    delay(200); // Độ trễ để tránh nhấn đúp quá nhanh
  }
  // Cập nhật lại trạng thái của nút chế độ
  lastModeButtonState = currentModeButtonState;
  // Cập nhật và gửi dữ liệu
  // Joystick
  data.x = analogRead(VRX);
  data.y = analogRead(VRY);
  data.swi = digitalRead(SW) == HIGH ? 0 : 1;
  data.speed = encoderPos;
  // Reset giá trị nút điều hướng
  data.front = 0;
  data.back = 0;
  data.left = 0;
  data.right = 0;
  // Xử lý nút điều hướng
  if (digitalRead(BTN_FWD) == HIGH) {
    data.front = 1;
    data.back = 0;
    data.left = 0;
    data.right = 0;
  } else if (digitalRead(BTN_REV) == HIGH) {
    data.back = 1;
    data.front = 0;
    data.left = 0;
    data.right = 0;
  } else if (digitalRead(BTN_LEF) == HIGH) {
    data.left = 1;
    data.back = 0;
    data.front = 0;
    data.right = 0;
  } else if (digitalRead(BTN_RIT) == HIGH) {
    data.right = 1;
    data.back = 0;
    data.left = 0;
    data.front = 0;
  }
  data.mode = modeState;
  // Kiểm tra nút chuyển chế độ
  Serial.print("Mode:  ");
  Serial.println(data.mode);
  // In ra Serial để debug
  Serial.print(data.x);     Serial.print("\t");
  Serial.print(data.y);     Serial.print("\t");
  Serial.print(data.swi);   Serial.print("\t");
  Serial.print(data.front); Serial.print("\t");
  Serial.print(data.back);  Serial.print("\t");
  Serial.print(data.left);  Serial.print("\t");
  Serial.print(data.right); Serial.print("\t");
  Serial.print(data.speed); Serial.println();
  // Gửi dữ liệu
  esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));
  delay(20);  // Độ trễ để tránh quá tải
}
