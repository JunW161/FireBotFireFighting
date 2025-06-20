
#include <ESP32Servo.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
// Chân điều khiển động cơ
#define in1 33
#define in2 32
#define in3 27
#define in4 26
#define spd 5
#define rely1 4
// Chân điều khiển servo (servo1 & 2: vòi nước servo 3 siêu âm)
#define sev1 14
#define sev2 13
#define sev3 22
// Cảm biến siêu âm
#define trigPin 18
#define echoPin 19
// cam biến nhiệt 
#define TEMP_SENSOR_ANALOG_PIN 35   // A0 → GPIO35
#define TEMP_SENSOR_DIGITAL_PIN 21  // D0 → GPIO21
// Servo 
Servo s1, s2, s3;
 
int sa1 = 130, sa2 = 80, sa3 = 90;
// Deadzone cho joystick
int deadzoneX = 80;
int deadzoneY = 80;
int speedValue = 190;  // Giá trị tốc độ mặc định
// PWM
const int freq = 30000;
const int pwmChannel = 2;
const int resolution = 8;
// WiFi UDP
const char *ssid = "Wifimode";
const char *password = "12345678";
WiFiUDP Udp;
unsigned int localPort = 8888;
char incomingPacket[255];
bool wifiConnected = false;  // Cờ đánh dấu Wi-Fi đã kết nối chưa
const int FIRE_TEMP_THRESHOLD = 60;     // nếu analogRead nhiệt độ  <60 thì xem là có lửa
const int AVERAGE_TEMP_THRESHOLD = 3200; //
const int SAFE_TEMP_THRESHOLD = 4000;     // nếu analogRead nhiệt độ > 3800 thì xem là đã an toàn
// Biến nhận từ PC
int fireDetected = 0;
int fireX = 0;
int fireY = 0;
int scanStep = 0;
int count =0;
int rawAnalogOut = 4095 ;
int distance =0 ;
// Cấu trúc dữ liệu điều khiển
typedef struct struct_message {
  int x;      // joystick X (servo vòi nước)
  int y;      // joystick Y (servo vòi nước)
  int swi;    // bật/tắt relay phun nước
  int front;
  int back;
  int left;
  int right;
  int speed;  // tốc độ động cơ
  int mode;   // 0: PC, 1: tay
} struct_message;
struct_message data; 
// 🚗 Đi tới
void moveForward() {
  digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);  digitalWrite(in4, HIGH);
}
// 🛑 Dừng xe
void stopMotors() {
  digitalWrite(in1, LOW);  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);  digitalWrite(in4, LOW);
}
// 🔄 Quay trái (bánh trái lùi, phải tiến)
void turnLeft() {
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);  digitalWrite(in4, LOW);
}
// 🔁 Quay phải (bánh phải lùi, trái tiến)
void turnRight() {
  digitalWrite(in1, HIGH);  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
}
// 🔙 Đi lùi
void moveBackward() {
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
}
// Hàm đọc khoảng cách từ cảm biến siêu âm
float readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.034 / 2;
  delay(150);
  return distance;
}
// Gọi để kiểm tra nhiệt độ
void  checkTemperature() {
  
  rawAnalogOut = analogRead(TEMP_SENSOR_ANALOG_PIN);
  delay(100);
  rawAnalogOut = analogRead(TEMP_SENSOR_ANALOG_PIN);
  delay(100);
  rawAnalogOut = analogRead(TEMP_SENSOR_ANALOG_PIN);
  delay(100);
  rawAnalogOut = analogRead(TEMP_SENSOR_ANALOG_PIN);
  delay(100);
}
//nhận thông tin từ pc 
void receiveDataFromPC() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) incomingPacket[len] = 0;
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, incomingPacket);
    if (error) {
      Serial.print("JSON error: ");
      Serial.println(error.c_str());
      return;
    }
    // nhận thông tin from JSON
    fireDetected = doc["fire"];
    fireX = doc["x"];
    fireY = doc["y"];
    // Debug output
    Serial.print("🔥 fireDetected: ");
    Serial.print(fireDetected);
    Serial.print(" | X: ");
    Serial.print(fireX);
    Serial.print(" | Y: ");
    Serial.println(fireY);
  }
  else
  {
    fireDetected = 0;
    fireX = 0;
    fireY = 0;
  }
}
// hàm tránh vật cản 
void avoidObstacle() {
  Serial.print("vào hàm voidObstacle... ");
  // Dừng xe
  stopMotors();
  delay(10);
  // Quay servo bên trái (servo3)
  sa3 = 165;  // Quay servo3 sang trái
  s3.write(sa3);
  delay(500);  // Quay servo một lúc để kiểm tra
  float leftDistance = readDistanceCM();  // Kiểm tra khoảng cách bên phải
  delay(100);
  leftDistance = readDistanceCM();
  delay(100);
  leftDistance = readDistanceCM();
  delay(100);
  leftDistance = readDistanceCM();
  delay(100);
  leftDistance = readDistanceCM();
  delay(100);
  // Quay servo bên phải (servo3)
  sa3 = 30;  // Quay servo3 sang phải
  s3.write(sa3);
  delay(500);  // Quay servo một lúc để kiểm tra
  float rightDistance = readDistanceCM();  // Kiểm tra khoảng cách bên trái
  delay(100);
  rightDistance = readDistanceCM();
  delay(100);
  rightDistance = readDistanceCM();
  delay(100);
  rightDistance = readDistanceCM();
  delay(100);
  rightDistance = readDistanceCM();
  delay(100);
  
  Serial.print(" khoảng cách bên phải: ");
  Serial.println(rightDistance);
   
  // trả về góc siêu âm
  sa3 = 90;
  s3.write(sa3);
  delay(700);
  Serial.print(" khoảng cách bên trái: ");
  Serial.println(leftDistance);
  // Dựa vào khoảng cách, quyết định quay sang bên có khoảng cách lớn hơn
  if (rightDistance > leftDistance && rightDistance >15) {
    Serial.println("Turning right...");
    // Quay phải
    turnRight();
    delay(400);
    // fix nhiễu
    stopMotors();
    delay(50);
    // di tien
    moveForward();
    delay(600);
    stopMotors();
    delay(50);
    // quay trái
    turnLeft();
    delay(700);
    // fix nhiễu
    stopMotors();
    delay(50);
  } 
  else if(rightDistance < 15 && leftDistance <15){
    Serial.println("Move Back ...");
    moveBackward();
    delay(500);
    stopMotors();
    delay(50);
    turnRight();
    delay(400);
    stopMotors();
    delay(50);
    moveForward();
    delay(600);
    stopMotors();
    delay(50);
    turnLeft();
    delay(600);
    stopMotors();
    delay(50);
  }
  else {
    Serial.println("Turning left...");
    // Quay trái
    turnLeft();
    delay(500);
    // fix nhiễu
    stopMotors();
    delay(50);
    // đi thẳng
    moveForward();
    delay(600);
    // fix nhiễu
    stopMotors();
    delay(50);
    // quay phải 
    turnRight();
    delay(600);
    // fix nhiễu
    stopMotors();
    delay(50);
  }
}
// đi tiến an toàn 
// 🚗 Đi tới
void moveForwardTracking() {
  distance = readDistanceCM();
  delay(100);
  distance = readDistanceCM();
  delay(50);
  distance = readDistanceCM();
  delay(100);
  distance = readDistanceCM();
  delay(100);
  Serial.print("distance: ");
  Serial.println(distance);
  if(distance< 25){
    Serial.println("Dừng xe...");
    stopMotors();
    delay(50);
    avoidObstacle();
    Serial.println("tránh vật...");
  }
  else if(distance > 25 && distance < 35)
  {
    moveForward();
    delay(500);
    Serial.println("tiến một đoạn 500 ");
    stopMotors();
    delay(20);
  }
  else{
    Serial.println("tiến một đoạn 700 ");
    moveForward();
    delay(700);
    stopMotors();
    delay(50);
  }
}
// check lửa
void checkFire() {
  Serial.println("🔥 checking fire!!!!");
  stopMotors();
  delay(100);
 // Xoay xe hướng về vị trí lửa cần 
  if(20 < fireX && fireX < 80) 
  {
    turnLeft();
    delay(600);
    stopMotors();
    delay(200);
    Serial.println("== ");
  } else if (fireX < 140 && fireX > 80) {
    turnRight();
    delay(400);
    stopMotors();
    delay(200);
    Serial.println("==== ");
  }
  else if(fireX > 140 && fireX < 200){
    turnLeft();
    delay(400);
    stopMotors();
    delay(200);
    Serial.println("==== == ");
  }
  else if(fireX > 200 && fireX < 280){
    turnRight();
    delay(600);
    stopMotors();
    delay(200);
    Serial.println("====== === ");
  }
  else{
    stopMotors();
    delay(300);
    Serial.println("========================== ");
  }
  
  while (true) { 
    checkTemperature();
    checkTemperature();
    delay(400);
    Serial.print("🌡️ Nhiệt độ (analog): ");
    Serial.println(rawAnalogOut);
    if (rawAnalogOut < FIRE_TEMP_THRESHOLD) {
      stopMotors();
      delay(500);
      Serial.println("🔥 Lửa gần, bắt đầu dập!");
      // thay đổi theo firex,y
      sa2 = 90;   
      s1.write(sa1);
      s2.write(sa2);
      delay(800);
      // Phun nước
      digitalWrite(rely1, HIGH);
      delay(2000);
      digitalWrite(rely1, LOW);
      delay(10);
      sa1 = 130;
      sa2 = 80;
      s1.write(sa1);
      s2.write(sa2);
      delay(600);
      // Kiểm tra sau khi phun
      while (true) { 
        checkTemperature();
        checkTemperature();
        delay(200);
        if (rawAnalogOut > SAFE_TEMP_THRESHOLD) {
          Serial.println("✅ Đã dập lửa!");
          fireDetected =0;
          rawAnalogOut = 4095; // 
          count = 9;
          break;
        } else {
          Serial.println("⏳ Lửa chưa tắt, điều chỉnh vòi...");
          digitalWrite(rely1, HIGH);
          sa1 = sa1 + 10;
          sa2 = 70;
          s1.write(sa1);
          s2.write(sa2);
          delay(800);
          sa1 = sa1 -30;
          sa2 =65;
          s1.write(sa1);
          s2.write(sa2);
          delay(800);
          sa1 = sa1 + 40;
          sa2 = 100;
          s1.write(sa1);
          s2.write(sa2);
          delay(1200);
          digitalWrite(rely1, LOW);
          sa1 = 130;
          sa2 = 80;
          s1.write(sa1);
          s2.write(sa2);
          delay(600);
        }
      }
    } else if(rawAnalogOut > FIRE_TEMP_THRESHOLD && rawAnalogOut < AVERAGE_TEMP_THRESHOLD){
      Serial.println("🔥fire ở cách một đoạn...");
      moveForward();
      delay(300);
      stopMotors();
      delay(400);
      Serial.println(" đang tiến tới fire 300...");
    }
    else {
      Serial.println("🤔 Chưa phát hiện đủ nhiệt độ, tiếp tục kiểm tra...");
      distance = readDistanceCM();
      delay(100);
      distance = readDistanceCM();
      delay(100);
      distance = readDistanceCM();
      delay(100);
      distance = readDistanceCM();
      delay(100);
      distance = readDistanceCM();
      delay(200);
      Serial.print("📏 Khoảng cách: ");
      Serial.println(distance);
      if (distance < 15) {
        Serial.println("⚠️ Có vật cản phía trước!");
        moveBackward();
        delay(200);
        stopMotors();
        delay(100);
        continue;
      }
      else{
        moveForwardTracking();
        delay(10);
        stopMotors();
        delay(200);
      }
    }
    count ++;
    Serial.print(" biến count:  ");
    Serial.println(count);
    if(count < 8 && count > 4)
    {
      Serial.println("đã qua 4 lượt check lửa...");
      Serial.println("check lửa lần cuối...");
      receiveDataFromPC();
      Serial.println("waitting pc data...");
      delay(2000); //////////////////////////////////////
      Serial.print("fireDetected: ");
      Serial.println(fireDetected);
      if (15 < fireX < 80) {
        turnLeft();
        delay(600);
        stopMotors();
        delay(100);
        checkTemperature();
        delay(100);
      } else if (fireX < 140 && fireX > 80) {
        turnLeft();
        delay(400);
        stopMotors();
        delay(100);
        checkTemperature();
        delay(100);
      }
      else if(fireX > 140 && fireX < 200){
        turnRight();
        delay(400);
        stopMotors();
        delay(100);
        checkTemperature();
        delay(100);
      }
      else if(fireX > 200 && fireX < 280){
        turnLeft();
        delay(600);
        stopMotors();
        delay(100);
        checkTemperature();
        delay(100);      
      }
      else{
        stopMotors();
        delay(200);
        checkTemperature();
        delay(100);
      }  
      checkTemperature();
      delay(100);
      if(fireDetected ==1 || rawAnalogOut < SAFE_TEMP_THRESHOLD )
      {
        Serial.println("đang phát hiện lửa, check lại ..");
        fireDetected = 1;
        count = 0;
      }
      else{
        Serial.println("Tín hiệu sai thoát check fire...");
        count =0;
        break;
      }
    }
    else if(count > 8){
      Serial.print("đã dập lửa thoát check fire....");
      count =0;
      break;
    }
  }
  stopMotors();
  delay(500);
  fireDetected = 0; // Reset cờ sau xử lý
}
// Xử lý dữ liệu từ ESP-NOW
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));
}
void setup() {
  Serial.begin(115200);
  
  // Setup ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Lỗi khởi tạo ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  // Thiết lập chân
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(spd, OUTPUT);
  pinMode(rely1, OUTPUT);
  pinMode(TEMP_SENSOR_ANALOG_PIN, INPUT);
//  pinMode(TEMP_SENSOR_DIGITAL_PIN, INPUT);
  // Servo
  s1.attach(sev1);
  s2.attach(sev2);
  s3.attach(sev3);
  s1.write(sa1);
  s2.write(sa2);
  s3.write(sa3);
  // PWM setup
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(spd, pwmChannel);
  // Cảm biến siêu âm
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  data.mode = 1;  // Mặc định là chế độ tay khi khởi động
}
void loop() {
  if (data.mode == 1) {  // Điều khiển tay
    Serial.println("đã vào mode1");
    ledcWrite(pwmChannel, data.speed);
    // Điều khiển relay (sửa lại logic)
    digitalWrite(rely1, data.swi == 1 ? HIGH : LOW);
    
    // Điều khiển động cơ (chỉ 1 hướng được chạy 1 lần)
    if (data.front == 1) {
      moveForward();
      
    }
    else if (data.back == 1) {
      moveBackward();
    }
    else if (data.right == 1) {
      turnRight();
    }
    else if (data.left == 1) {
      turnLeft();
    }
    else {
      stopMotors();
    } 
    // Servo vòi nước điều khiển bằng joystick
    int increment = 3;
    if (abs(data.x - 1900) > deadzoneX) {
      sa1 += (data.x < 1900 && sa1 < 180) ? increment : (data.x > 1900 && sa1 > 0) ? -increment : 0;
      s1.write(sa1);
      delay(50);
    }
    if (abs(data.y - 1850) > deadzoneY) {
      sa2 -= (data.y < 1850 && sa2 > 0) ? increment : (data.y > 1850 && sa2 < 180) ? -increment : 0;
      s2.write(sa2);
      delay(50);
    }
  }
  else if (data.mode == 0) {  // Điều khiển PC
    //
    if (!wifiConnected) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      Serial.println("Connecting to WiFi...");
      unsigned long startAttemptTime = millis();   
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 3000) {
        delay(500);
        Serial.print(".");
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected.");
        wifiConnected = true;
        Udp.begin(localPort);
        Serial.println("UDP server started");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());  // In địa chỉ IP của ESP32
        Serial.print("MAC Address: ");
        Serial.println(WiFi.macAddress());  // In địa chỉ MAC của ESP32
      } else {
        Serial.println("\nWiFi failed.");
      }
    }


    ledcWrite(pwmChannel, speedValue);
    checkTemperature();
    delay(50);
    
    //
    if(rawAnalogOut < SAFE_TEMP_THRESHOLD ){
      Serial.println("check fire...");
      checkFire();
      scanStep =0;
    }
    receiveDataFromPC();
    Serial.println("waitting pc data...");
    delay(2000);
    Serial.print("fireDetected: ");
    Serial.println(fireDetected);
    checkTemperature();
    delay(50);
    Serial.print("🌡️ Nhiệt độ (analog): ");
    Serial.println(rawAnalogOut);
    Serial.print("fire x :  ");
    Serial.println(fireX);
    Serial.print("fire y :  ");
    Serial.println(fireY);
    if(fireDetected ==1 || rawAnalogOut < SAFE_TEMP_THRESHOLD ){
      Serial.println("check fire...");
      checkFire();
      scanStep =0;
    }
    else{
      if(scanStep <4)
      {
        Serial.println("vao scanstep2 ");
        checkTemperature();
        checkTemperature();
        delay(10);
        if(rawAnalogOut < SAFE_TEMP_THRESHOLD )
        {
          Serial.println("check fire...");
          checkFire();
          scanStep =0;
        }
        else{
          Serial.println("quay check góc...");
          turnRight();
          delay(600);
          stopMotors();
          delay(50);
          scanStep +=1;
        }
      }
      else{
        int r = random(0, 3);
        switch (r) {
          case 0:
            Serial.println("tranking thẳng");
            moveForwardTracking();
            break;
          case 1:
            Serial.println("left tracking");
            turnLeft();
            delay(600);
            stopMotors();
            delay(50);
            checkTemperature();
            delay(50);
            if(rawAnalogOut < SAFE_TEMP_THRESHOLD )
            {
              checkFire();
              scanStep =0;
            }
            else{
              moveForwardTracking();
            }
            
            break;
          case 2:
            Serial.println("right tracking");
            turnRight();
            delay(600);
            stopMotors();
            delay(50);
            checkTemperature();
            delay(50);
            if(rawAnalogOut < SAFE_TEMP_THRESHOLD )
            {
              Serial.println("check fire...");
              checkFire();
              scanStep =0;
            }
            else{
              moveForwardTracking();
            }
            break;
        }
        scanStep = 0;
      }
    }
  }
}
