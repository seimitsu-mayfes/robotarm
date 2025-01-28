#include <ESP32Servo.h>

Servo servo1, servo2, servo3, servo4, servo5, servo6;

// サーボモーターのピン定義
const int servoPin1 = 25; // SG-5010　z軸中心回転　手首部分
const int servoPin2 = 26; // SG-5010　y軸中心回転　アームのハンド
const int servoPin3 = 27; // RDS3218　z軸中心回転　デフォルト(正面)角度= 90度
const int servoPin4 = 14; // RDS3218　x軸中心回転　デフォルト(真っ直ぐ)角度=90度　第一関節
const int servoPin5 = 12; // RDS3218　x軸中心回転　デフォルト(真っ直ぐ)角度=90度　第二関節　
const int servoPin6 = 13; // RDS3218　x軸中心回転  デフォルト(真っ直ぐ)角度=90度　第三関節

// ボタンのピン定義
const int buttonPin = 2;

// サーボモーターの稼働範囲定義
const int minAngle1 = 0, maxAngle1 = 180; // SG-5010
const int minAngle2 = 0, maxAngle2 = 180; // SG-5010
const int minAngle3 = 0, maxAngle3 = 270; // RDS3218
const int minAngle4 = 0, maxAngle4 = 270; // RDS3218
const int minAngle5 = 0, maxAngle5 = 270; // RDS3218
const int minAngle6 = 0, maxAngle6 = 270; // RDS3218

// 動作制御用の変数
bool isMoving = false;
int targetAngles[6] = {0, 0, 0, 0, 0, 0};
int currentAngles[6] = {0, 0, 0, 0, 0, 0};

void setup() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // SG-5010 サーボ
  servo1.setPeriodHertz(50);
  servo1.attach(servoPin1, 1000, 2000);
  servo2.setPeriodHertz(50);
  servo2.attach(servoPin2, 1000, 2000);

  // RDS3218 サーボ
  servo3.setPeriodHertz(50);
  servo3.attach(servoPin3, 500, 2500);
  servo4.setPeriodHertz(50);
  servo4.attach(servoPin4, 500, 2500);
  servo5.setPeriodHertz(50);
  servo5.attach(servoPin5, 500, 2500);
  servo6.setPeriodHertz(50);
  servo6.attach(servoPin6, 500, 2500);

  // ボタンのセットアップ
  pinMode(buttonPin, INPUT_PULLUP);

  // シリアル通信の開始
  Serial.begin(115200);
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    // ボタンが押されたときの動作
    moveServos(30, 90, 120, 150, 180, 210);
    delay(1000); // デバウンス
  }

  // Bluetoothからの入力をシミュレート
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

  // サーボの動きを更新
  updateServoPositions();
}

void moveServos(int angle1, int angle2, int angle3, int angle4, int angle5, int angle6) {
  targetAngles[0] = constrain(angle1, minAngle1, maxAngle1);
  targetAngles[1] = constrain(angle2, minAngle2, maxAngle2);
  targetAngles[2] = constrain(angle3, minAngle3, maxAngle3);
  targetAngles[3] = constrain(angle4, minAngle4, maxAngle4);
  targetAngles[4] = constrain(angle5, minAngle5, maxAngle5);
  targetAngles[5] = constrain(angle6, minAngle6, maxAngle6);
  isMoving = true;
}

void updateServoPositions() {
  if (isMoving) {
    bool allReached = true;
    for (int i = 0; i < 6; i++) {
      if (currentAngles[i] < targetAngles[i]) {
        currentAngles[i]++;
        allReached = false;
      } else if (currentAngles[i] > targetAngles[i]) {
        currentAngles[i]--;
        allReached = false;
      }
    }
    writeServoAngles();
    if (allReached) {
      isMoving = false;
    }
    delay(15); // サーボの動きを滑らかにするための遅延
  }
}

void writeServoAngles() {
  servo1.write(currentAngles[0]);
  servo2.write(currentAngles[1]);
  servo3.write(currentAngles[2]);
  servo4.write(currentAngles[3]);
  servo5.write(currentAngles[4]);
  servo6.write(currentAngles[5]);
}

void processCommand(String command) {
  if (command.startsWith("MOVE ")) {
    int angles[6];
    int index = 0;
    command = command.substring(5); // "MOVE "の部分を削除
    while (command.length() > 0 && index < 6) {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex == -1) {
        angles[index++] = command.toInt();
        break;
      } else {
        angles[index++] = command.substring(0, spaceIndex).toInt();
        command = command.substring(spaceIndex + 1);
      }
    }
    if (index == 6) {
      moveServos(angles[0], angles[1], angles[2], angles[3], angles[4], angles[5]);
    }
  } else if (command == "STOP") {
    isMoving = false;
  } else if (command == "START") {
    isMoving = true;
  }
}
