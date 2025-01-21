#include <Stepper.h>

// ステッピングモーターの設定
const int around_step = 200; // 1回転のステップ数
Stepper stepper1(around_step, 13, 12, 14, 27);
Stepper stepper2(around_step, 26, 25, 33, 32);

// ステップ数と回転スピード
int step = 10;
int speed = 50;

void setup() {
  // ステッピングモーターの初期設定
  stepper1.setSpeed(speed);
  stepper2.setSpeed(speed);
}

void loop() {
  // ステッピングモーター1の回転
  stepper1.step(step);
  
  // ステッピングモーター2の回転
  stepper2.step(step);
  
  //delay(100); // 0.1秒の遅延（必要に応じて調整）
}
