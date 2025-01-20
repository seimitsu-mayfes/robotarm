#include <Stepper.h>

// 1回転ステップ数（SM-42BYG011の場合）
int around_step = 200;

int sw_pin = 3;   // スイッチの入力Pin
int led_pin = 7;  // LEDの出力Pin
volatile int sw;  // SWの状態

// ボタン一回を押して進むステップ数
// 正転：10、反転：-10
int step = 10;

// 回転スピード（10：遅い、50：早い）
int speed =200;   

void sw_on(void);

// stepper インスタンス生成
Stepper stepper(around_step, 8,9,10,11);

void setup() {

  pinMode(sw_pin,INPUT);
  pinMode(led_pin,OUTPUT);

  // ボタンの変化で割り込み処理開始
  attachInterrupt(1,sw_on, CHANGE);

}

void loop() {

  // スイッチを長押し中かどうか
  sw = digitalRead(sw_pin);

  // スイッチがONのときのみモーターを回転
  if(sw == 1) {

    //ステップ数（正転）
    stepper.step(step);

    // 回転スピード
    stepper.setSpeed(speed);   

  }

}

// スイッチが押されたときの割り込み処理
void sw_on(void) {

  if(sw == 0) {
    sw = 1;
    digitalWrite(led_pin,HIGH);
  }
   else {
    digitalWrite(led_pin,LOW);
  }

}