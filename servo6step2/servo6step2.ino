#include <Stepper.h>
#include <Servo.h>

// ステッピングモーターの設定
const int around_step = 200; // 1回転のステップ数
Stepper stepper1(around_step, 8, 9, 10, 11); // ステッピングモーター1
Stepper stepper2(around_step, 12, 13, 14, 15); // ステッピングモーター2

// サーボモーターの設定
Servo servo1; // サーボモーター1
Servo servo2; // サーボモーター2
Servo servo3; // サーボモーター3
Servo servo4; // サーボモーター4

int sw_pin = 3;   // スイッチの入力Pin
int led_pin = 7;  // LEDの出力Pin
volatile int sw;  // SWの状態

// ボタン一回を押して進むステップ数
int step = 10;

// 回転スピード（10：遅い、50：早い）
int speed = 200;

void sw_on(void);

void setup() {
  pinMode(sw_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  // ボタンの変化で割り込み処理開始
  attachInterrupt(digitalPinToInterrupt(sw_pin), sw_on, CHANGE);

  // サーボモーターの初期化
  servo1.attach(4); // サーボ1をGPIO4に接続
  servo2.attach(5); // サーボ2をGPIO5に接続
  servo3.attach(6); // サーボ3をGPIO6に接続
  servo4.attach(7); // サーボ4をGPIO7に接続

  // ステッピングモーターの初期設定
  stepper1.setSpeed(speed);
  stepper2.setSpeed(speed);
}

void loop() {
  // スイッチを長押し中かどうか
  sw = digitalRead(sw_pin);

  // スイッチがONのときのみモーターを回転
  if (sw == HIGH) {
    // ステッピングモーターの回転（正転）
    stepper1.step(step);
    stepper2.step(step);

    // サーボモーターの動作（例として角度を設定）
    servo1.write(90); // サーボ1を90度に設定
    servo2.write(90); // サーボ2を90度に設定
    servo3.write(90); // サーボ3を90度に設定
    servo4.write(90); // サーボ4を90度に設定

    delay(500); // モーターとサーボが動作する時間待機
  } else {
    // スイッチがOFFの場合、サーボは元の位置に戻す例（0度）
    servo1.write(0);
    servo2.write(0);
    servo3.write(0);
    servo4.write(0);
    
    delay(500); // モーターとサーボが動作する時間待機
  }
}

// スイッチが押されたときの割り込み処理
void sw_on(void) {
  if (sw == LOW) {
    sw = HIGH;
    digitalWrite(led_pin, HIGH);
  } else {
    digitalWrite(led_pin, LOW);
    sw = LOW;
  }
}
