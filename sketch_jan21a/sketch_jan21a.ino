#include <Stepper.h>

// ステッピングモーターの設定
const int around_step = 200; // 1回転のステップ数
Stepper stepper(around_step, 14, 27, 26, 25);

// ステップ数と回転スピード
int step = 10;
int speed = 200;

void setup() {
  // ステッピングモーターの初期設定
  stepper.setSpeed(speed);
}

void loop() {
  // ステッピングモーターの回転
  stepper.step(step);
}
