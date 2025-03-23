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
const int servoPins[6] = {17, 19, 21, 22, 26, 32};

//注意!:RDS3218は目標角度×2/3を入力する必要がある。最初の4つがRDS3218、後の2つがSG-5010。
// サーボモーターの稼働範囲定義（各サーボの最小角度）
const int minAngles[6] = {0, 30, 30, 30, 0, 0};
// サーボモーターの稼働範囲定義（各サーボの最大角度）
const int maxAngles[6] = {270, 240, 240, 240, 180, 180};

// サーボモーターのデフォルト位置（各サーボの初期角度）
const int defaultAngles[6] = {135, 200, 30, 45, 90, 90}; 

// 現在の角度を保存する配列（各サーボの現在の角度を記録）
int currentAngles[6] = {135, 200, 30, 45, 90, 90};

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
        Serial.println("受信したコマンド: " + value); // 受信したコマンドをシリアル出力
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
      // 最初の4つのサーボ（RDS3218）の設定
      servos[i].attach(servoPins[i], 500, 2500);
    } else {
      // 残りの2つのサーボ（SG-5010）の設定
      servos[i].attach(servoPins[i], 500, 2500);
    }
    // サーボをデフォルト位置に移動（RDS3218サーボは角度調整が必要）
    int adjustedDefaultAngle = (i < 4) ? round((defaultAngles[i] * 2.0) / 3.0) : defaultAngles[i];
    servos[i].write(adjustedDefaultAngle);
    currentAngles[i] = defaultAngles[i];
    Serial.printf("サーボ %d を角度 %d に初期化 (調整後 %d)\n", i + 1, defaultAngles[i], adjustedDefaultAngle);
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
  Serial.println("BLEデバイスがペアリング待機中");
}

void loop() {
  // BLE接続状態の管理
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 接続が切れた後、再アドバタイジングを開始するまでの遅延
    pServer->startAdvertising(); // 再アドバタイジングの開始
    Serial.println("アドバタイジング開始");
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
      // RDS3218サーボの場合、角度を2/3倍して書き込む
      int adjustedAngle = (id < 4) ? round((angle * 2.0) / 3.0) : angle;
      servos[id].write(adjustedAngle);  // サーボを指定された角度に動かす
      currentAngles[id] = angle;  // 現在の角度を更新
      Serial.printf("サーボ %d を角度 %d に移動 (調整後 %d)\n", id + 1, angle, adjustedAngle);
    } else {
      Serial.printf("エラー: 角度 %d はサーボ %d の範囲外です\n", angle, id + 1);
    }
  } else {
    Serial.printf("エラー: 無効なサーボID %d\n", id + 1);
  }
}

// 受信したコマンドを処理する関数
void processCommand(String command) {
  // 空白で区切られた6つの数値を解析
  int angles[6];
  int count = 0;
  int lastIndex = 0;
  int nextIndex = 0;
  
  // コマンド文字列を解析して6つの角度を取得
  while (nextIndex >= 0 && count < 6) {
    nextIndex = command.indexOf(' ', lastIndex);
    String angleStr;
    
    if (nextIndex < 0) {
      // 最後の数値
      angleStr = command.substring(lastIndex);
    } else {
      // 中間の数値
      angleStr = command.substring(lastIndex, nextIndex);
      lastIndex = nextIndex + 1;
    }
    
    if (angleStr.length() > 0) {
      angles[count] = angleStr.toInt();
      count++;
    }
  }
  
  // 6つの角度が正しく取得できたかチェック
  if (count == 6) {
    Serial.println("6つのサーボ角度を設定します:");
    for (int i = 0; i < 6; i++) {
      Serial.printf("サーボ %d: %d度\n", i + 1, angles[i]);
      moveServo(i, angles[i]);
    }
    Serial.println("全てのサーボ角度を設定しました");
  } else {
    Serial.printf("エラー: 6つの角度が必要ですが、%d個しか取得できませんでした\n", count);
    Serial.println("正しい形式: '角度1 角度2 角度3 角度4 角度5 角度6'");
  }
} 