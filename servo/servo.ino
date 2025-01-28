#include <ESP32Servo.h>

Servo servo1;
const int servoPin1 = 25;

void setup() {
  ESP32PWM::allocateTimer(0);
  servo1.setPeriodHertz(150);  // 標準的な50Hz
  servo1.attach(servoPin1, 500, 2500);  // サーボの最小/最大パルス幅を調整
}

void loop() {

  servo1.write(180);
  delay(5000);  // 2秒待機

  // servo1.write(0);
  // delay(3000);  // 2秒待機

  // servo1.write(90);
  // delay(1000);  // 2秒待機

  // servo1.write(135);
  // delay(1000);  // 2秒待機


  // servo1.write(270);
  // delay(2000);  // 2秒待機
}


//移動が終わる時間を計算しないといけない。
//データシートに記載されている「0.15sec/60度」という情報を使用して、移動時間を計算するか
//マイルストーン方式：大きな角度変化を小さな変化に分割し、各ステップで短い遅延を入れる方法です。