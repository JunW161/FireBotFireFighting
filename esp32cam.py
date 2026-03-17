
import cv2
import numpy as np
import socket
import time
import json
# === Cấu hình === 
stream_url = "http://172.20.10.2:81/stream"   # Địa chỉ stream từ ESP32-CAM
esp32_ip = "172.20.10.3"                 # Địa chỉ ESP32 chính
esp32_port = 12345                              # Cổng UDP
# === Tạo socket UDP ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(1.0)  # timeout ngắn để không bị kẹt
# === Buffer xác nhận lửa ổn định ===
fire_buffer = []
BUFFER_SIZE = 5
CONFIRM_THRESHOLD = 4
last_sent_time = 0
SEND_INTERVAL = 0.5    # chỉ gửi mỗi 1giây 
fire_was_sent = False
# === Mở stream ===
cap = cv2.VideoCapture(stream_url)
def update_fire_buffer(is_fire):
    fire_buffer.append(is_fire)
    if len(fire_buffer) > BUFFER_SIZE:
        fire_buffer.pop(0)
    return fire_buffer.count(True) >= CONFIRM_THRESHOLD
def detect_fire(frame):
    frame = cv2.resize(frame, (320, 240))
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    # Ngưỡng HSV cho lửa
    lower_fire = np.array([0, 95, 105])
    upper_fire = np.array([35, 255, 255])
    mask = cv2.inRange(hsv, lower_fire, upper_fire)
    mask = cv2.GaussianBlur(mask, (5, 5), 0)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    fire_detected = False
    fire_center = None
    max_area = 0
    largest_contour = None
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area > 150:
            if area > max_area:
                max_area = area
                largest_contour = cnt
                fire_detected = True
    if fire_detected and largest_contour is not None:
        x, y, w, h = cv2.boundingRect(largest_contour)
        fire_center = (int(x + w / 2), int(y + h / 2))
        cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)
        cv2.circle(frame, fire_center, 5, (0, 255, 255), -1)
        cv2.putText(frame, "FIRE!", (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,0,255), 2)
    return frame, fire_detected, fire_center
print("🚀 Bắt đầu nhận stream và phát hiện lửa...")
while True:
    ret, frame = cap.read()
    if not ret:
        print("⚠️ Không lấy được hình ảnh từ stream")
        break
    processed_frame, is_fire, center = detect_fire(frame)
    fire_confirmed = update_fire_buffer(is_fire)
    now = time.time()
    # Gửi thông tin khi lửa đủ số lần xác nhận (tức ngưỡng CONFIRM_THRESHOLD)
    if fire_confirmed and center is not None:
        if now - last_sent_time > SEND_INTERVAL:
            data = {
                "type": "fire",
                "x": center[0],
                "y": center[1]
            }
            msg = json.dumps(data)
            try:
                sock.sendto(msg.encode(), (esp32_ip, esp32_port))
                print(f"🔥 Đã gửi: {msg}")
                fire_was_sent = True
                last_sent_time = now
            except Exception as e:
                print(f"❌ Lỗi gửi UDP: {e}")
                fire_was_sent = False
            # Debug: Kiểm tra xem dữ liệu đã được gửi thành công chưa
            if fire_was_sent:
                print("✅ Gửi dữ liệu thành công đến ESP32!")
            else:
                print("⚠️ Gửi dữ liệu không thành công!")
    # Hiển thị kết quả
    cv2.imshow("Fire Detection", processed_frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
cap.release()
cv2.destroyAllWindows()
