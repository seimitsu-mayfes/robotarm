#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

// BLE関連の変数
BLEServer* pServer = NULL; // BLEサーバーのインスタンス
BLECharacteristic* pCharacteristic = NULL; // BLEキャラクタリスティックのインスタンス
bool deviceConnected = false; // デバイス接続状態を示すフラグ
bool oldDeviceConnected = false; // 前回の接続状態を記録するフラグ

// BLEサービスとキャラクタリスティックのUUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// サーボモーターの定義（6つのサーボを配列で管理）
Servo servos[6];

// サーボモーターのピン定義（各サーボのGPIOピン番号）
const int servoPins[6] = {25, 26, 27, 14, 12, 13};

//注意!:RDS3218は目標角度×2/3を入力する必要がある。
// サーボモーターの稼働範囲定義（各サーボの最小角度）
const int minAngles[6] = {0, 30, 30, 30, 0, 0};
// サーボモーターの稼働範囲定義（各サーボの最大角度）
const int maxAngles[6] = {180, 150, 150, 150, 180, 180};

// サーボモーターのデフォルト位置（各サーボの初期角度）
const int defaultAngles[6] = {90, 90, 90, 90, 90, 90}; 

// 現在の角度を保存する配列（各サーボの現在の角度を記録）
int currentAngles[6] = {0, 0, 0, 0, 0, 0};

// 受信したコマンドを処理する関数のプロトタイプ宣言
void processCommand(String command);

// BLEサーバーコールバッククラス：接続状態の管理
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true; // デバイスが接続されたときにフラグを設定
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false; // デバイスが切断されたときにフラグをリセット
    }
};

// BLEキャラクタリスティックコールバッククラス：データ受信時の処理
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str(); // 受信したデータをString型に変換
      if (value.length() > 0) {
        Serial.println("Received command: " + value); // 受信したコマンドをシリアル出力
        processCommand(value); // 受信したコマンドを処理
      }
    }
};

void setup() {
  Serial.begin(115200); // シリアル通信の初期化

  // ESP32のPWMタイマーの割り当て（サーボ制御に必要）
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // サーボモーターの初期化
  for (int i = 0; i < 6; i++) {
    servos[i].setPeriodHertz(50);  // 全てのサーボの周波数を50Hzに設定
    if (i < 4) {
      // 最初の2つのサーボ（SG-5010）の設定
      servos[i].attach(servoPins[i], 500, 2500);
    } else {
      // 残りの4つのサーボ（RDS3218）の設定
      servos[i].attach(servoPins[i], 1000, 2000);
    }
    // サーボをデフォルト位置に移動
    servos[i].write(defaultAngles[i]);
    currentAngles[i] = defaultAngles[i];
  }

  // BLEデバイスの初期化
  BLEDevice::init("ESP32 BLE Device");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // BLEサービスの作成
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // BLEキャラクタリスティックの作成
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  // サービスの開始
  pService->start();
  
  // アドバタイジングの設定と開始
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("BLE device is ready to pair");
}

void loop() {
  // BLE接続状態の管理
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 接続が切れた後、再アドバタイジングを開始するまでの遅延
    pServer->startAdvertising(); // 再アドバタイジングの開始
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

// 指定されたIDのサーボを指定された角度に動かす関数
void moveServo(int id, int angle) {
  if (id >= 0 && id < 6) {
    if (angle >= minAngles[id] && angle <= maxAngles[id]) {
      servos[id].write(angle);  // サーボを指定された角度に動かす
      currentAngles[id] = angle;  // 現在の角度を更新
      Serial.printf("Servo %d moved to angle %d\n", id + 1, angle);
      Serial.println("Angle set successfully."); // 角度を指定した後のログ
    } else {
      Serial.printf("Error: Angle %d is out of range for Servo %d\n", angle, id + 1);
    }
  } else {
    Serial.printf("Error: Invalid servo ID %d\n", id + 1);
  }
}

// 受信したコマンドを処理する関数
void processCommand(String command) {
  int id, angle;
  if (sscanf(command.c_str(), "%d %d", &id, &angle) == 2) {
    Serial.printf("Processing command: ID=%d, Angle=%d\n", id, angle);
    moveServo(id - 1, angle);  // IDは1から始まるが、配列は0から始まるため調整
  } else {
    Serial.println("Error: Invalid command format. Use 'ID ANGLE'");
  }
}
