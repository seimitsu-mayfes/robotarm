#include <ESP32Servo.h>

Servo servo1;
const int servoPin1 = 26;

void setup() {
  Serial.begin(115200);
  ESP32PWM::allocateTimer(0);
  servo1.setPeriodHertz(50);  // 標準的な50Hz
  servo1.attach(servoPin1, 500, 2500);  // サーボの最小/最大パルス幅を調整
}

void loop() {
  Serial.printf("Moved to angle: %d\n", 0);
  servo1.write(0);
  delay(5000);  // 5秒待機

  Serial.printf("Moved to angle: %d\n", 45);
  servo1.write(30);
  delay(3000);  // 3秒待機

  Serial.printf("Moved to angle: %d\n", 90);
  servo1.write(60);
  delay(3000);  // 3秒待機

  Serial.printf("Moved to angle: %d\n", 135);
  servo1.write(90);
  delay(3000);  // 3秒待機

  Serial.printf("Moved to angle: %d\n", 180);
  servo1.write(120);
  delay(3000);  // 3秒待機

  Serial.printf("Moved to angle: %d\n", 225);
  servo1.write(150);
  delay(3000);  // 3秒待機

  Serial.printf("Moved to angle: %d\n", 270);
  servo1.write(180);
  delay(3000);  // 3秒待機
}


//移動が終わる時間を計算しないといけない。
//データシートに記載されている「0.15sec/60度」という情報を使用して、移動時間を計算するか
//マイルストーン方式：大きな角度変化を小さな変化に分割し、各ステップで短い遅延を入れる方法です。