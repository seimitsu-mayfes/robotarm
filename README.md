# robotarm

esp32の注意点
35,34は入力専用
boot(書き込み)をするときは12ピンを外す必要がある。
https://lastminuteengineers.com/esp32-pinout-reference/

# PyEspBleConnect
pythonとESP32をbluetooth low energy で接続する方法


<br>
python3 -m venv env　仮想環境を作成<br>
source env/bin/activate 仮想環境を有効化<br>
pip install -r requirements.txt 必要なライブラリを仮想環境にインストール<br>


python3 BLECentral.py <br>

---

# FastAPIサーバーによるロボットアーム制御

## 起動方法

1. 仮想環境を有効化
    ```sh
    source env/bin/activate
    ```
2. 依存パッケージをインストール
    ```sh
    pip install -r requirements.txt
    ```
3. FastAPIサーバーを起動
    ```sh
    uvicorn robotarm_api:app --reload
    ```

## APIフロー

- サーバー起動時に一度だけBLE接続を確立します。
- `/action` エンドポイントにPOSTでアクション名（例: `greeting`）を送信すると、BLE経由でロボットアームにコマンドが送信されます。
- BLE接続が切断されていた場合は自動で再接続します。
- サーバー終了時にBLE接続を切断します。

## 動作確認例

```sh
curl -X POST http://localhost:8000/action -H "Content-Type: application/json" -d '{"action": "greeting"}'
```

## OpenAPIドキュメント

ブラウザで [http://localhost:8000/docs](http://localhost:8000/docs) にアクセスすると、APIのテストや確認ができます。

---

## servo6.ino と servo6-6.ino の違いについて

### 共通点
- どちらもESP32を使い、BLE（Bluetooth Low Energy）経由で6軸サーボモーターを制御するプログラムです。
- サーボモーターのピン配置や初期角度、稼働範囲、BLEサービスUUIDなどの基本構成は同じです。
- RDS3218（最初の4つ）とSG-5010（後ろ2つ）のサーボを混在して制御します。

### 主な違い

#### 1. サーボへのコマンド形式
- **servo6.ino**
  - コマンドは「サーボID 角度」の2つの数値（例: `1 90`）を受信し、指定IDのサーボを指定角度に動かします。
  - 1つのサーボのみを個別に動かす用途向けです。
- **servo6-6.ino**
  - コマンドは「角度1 角度2 角度3 角度4 角度5 角度6」の6つの数値（例: `90 120 30 45 90 90`）を空白区切りで一度に受信し、6つ全てのサーボを同時に動かします。
  - 複数サーボを一括で制御したい場合に便利です。

#### 2. サーボのattach設定
- **servo6.ino**
  - RDS3218（最初の4つ）は `attach(ピン, 500, 2500)`、SG-5010（後ろ2つ）は `attach(ピン, 1000, 2000)` でPWMパルス幅を分けて設定しています。
- **servo6-6.ino**
  - すべてのサーボで `attach(ピン, 500, 2500)` を使用しています。

#### 3. シリアル出力のメッセージ
- **servo6.ino**
  - 英語のログ（例: `Servo 1 initialized to angle 135 (adjusted to 90)`）やエラーメッセージが英語です。
- **servo6-6.ino**
  - 日本語のログ（例: `サーボ 1 を角度 135 に初期化 (調整後 90)`）やエラーメッセージが日本語です。

#### 4. コマンド解析方法
- **servo6.ino**
  - `sscanf`で2つの数値（IDと角度）を抽出。
- **servo6-6.ino**
  - 文字列を空白で分割し、6つの角度を配列に格納。

#### 5. 用途の違い
- **servo6.ino**
  - テストや個別サーボの動作確認、または1軸ずつ制御したい場合に適しています。
- **servo6-6.ino**
  - ロボットアームの全軸を一括で動かす用途や、動作パターンを一度に送信したい場合に適しています。

---

## 各種サンプルスケッチの説明

### servo/servo.ino
- 単一のサーボモーター（例：SG-5010など）をESP32で制御する基本サンプルです。
- サーボを0度、30度、60度、90度と順番に動かし、各角度で数秒停止します。
- サーボの動作確認やパルス幅調整のテストに最適です。

### encoder/encoder.ino
- ロータリーエンコーダのA/B相を読み取り、回転方向とカウントをシリアル出力するサンプルです。
- 割り込みを使ってエンコーダの変化を高精度にカウントします。
- ロボットアームの角度検出や回転量の計測などに利用できます。

### steppingmotor/steppingmotor.ino
- ステッピングモーター（例：SM-42BYG011）をボタン入力で制御するサンプルです。
- ボタンを押すと指定ステップ数だけモーターが回転し、LEDで状態を表示します。
- 割り込みを使ってボタン入力を検出し、モーターの正転・反転や速度調整が可能です。

### sketch_jan21a/sketch_jan21a.ino
- 2つのステッピングモーターを同時に制御するサンプルです。
- それぞれのモーターを同じステップ数・速度で回転させます。
- 複数軸の協調動作や基本的な動作テストに利用できます。

---

# ファイル構成の概要

## 1. ESP32に書き込む用のスケッチ群
これらはArduino IDEやPlatformIOなどを使ってESP32に直接書き込むプログラムです。主にロボットアームや各種モーター、エンコーダの制御を行います。

- **servo/servo.ino**：単一サーボモーターの基本動作確認用サンプル。
- **servo6/servo6.ino**：BLE経由で1軸ずつサーボを制御するプログラム。
- **servo6-6/servo6-6.ino**：BLE経由で6軸同時にサーボを制御するプログラム。
- **encoder/encoder.ino**：ロータリーエンコーダのカウント・回転方向検出サンプル。
- **steppingmotor/steppingmotor.ino**：ボタン入力でステッピングモーターを制御するサンプル。
- **sketch_jan21a/sketch_jan21a.ino**：2つのステッピングモーターを同時制御するサンプル。

これらのスケッチは、ESP32に書き込むことで各種ハードウェアの動作確認や制御が可能です。

## 2. BLE通信でデータを送るPythonファイル群
これらはPCやRaspberry PiなどからESP32にBLE経由でコマンドやデータを送信するためのPythonスクリプトです。

- **PyEspBleConnect/BLECentral.py**：PythonからESP32にBluetooth Low Energyで接続し、コマンドを送信するサンプルスクリプトです。
  - コマンドラインから「ID 角度」などのコマンドを入力し、ESP32に送信できます。
- **simulBLEcentral.py**：6つのサーボ角度をスペース区切りで入力し、BLE経由で一括送信できるスクリプトです。
  - servo6-6.inoなど6軸同時制御用のファームウェアと組み合わせて使います。
- **angle_sequence_sender.py**：JSONファイル（angle_sequences.json）に記述した複数の角度シーケンスを補間し、BLE経由で連続送信するスクリプトです。
  - ロボットアームの複雑な動作パターンを自動再生したい場合に便利です。
- **robotactionBLE.py**：robotaction.jsonに定義された「greeting」「wave」などの動作シーケンスを選択し、BLE経由でESP32に送信するスクリプトです。
  - コマンドラインからアクション名を入力することで、対応する動作パターンを実行できます。
- **robotaction.json**：robotactionBLE.pyが参照する動作パターン定義ファイルです。
  - 各アクション名ごとに6軸サーボの角度シーケンスが記述されており、ロボットアームの「挨拶」「手を振る」などの動きを定義します。
- **angle_sequences.json**：angle_sequence_sender.pyが参照する角度シーケンス定義ファイルです。
  - 任意の動作パターンを複数記述でき、補間処理を経て滑らかに再生できます。

これらのPythonファイルを使うことで、PCやWeb APIからロボットアームやモーターを遠隔制御したり、複雑な動作パターンを自動実行できます。

---

# ロボットアームを動かすための手順

1. **ESP32にファームウェアを書き込む**
   - Arduino IDEやPlatformIOなどで `servo6-6/servo6-6.ino` をESP32に書き込む。

2. **Python仮想環境の準備と依存パッケージのインストール**
   - ターミナルでプロジェクトディレクトリに移動し、以下を実行：
     ```sh
     python3 -m venv env
     source env/bin/activate
     pip install -r requirements.txt
     ```

3. **BLEサーバーAPI（バックエンド）の起動**
   - `robotarm_api.py`（FastAPIサーバー）を起動：
     ```sh
     uvicorn robotarm_api:app --reload
     ```
   - これでPCからBLE経由でESP32にコマンドを送るサーバーが立ち上がります。

4. **フロントエンドサーバーの起動**
   - `robotarmfrontend` ディレクトリに移動し、以下を実行：
     ```sh
     npm install
     npm run dev
     ```
   - ブラウザで表示されるURLにアクセスし、ロボットアームの操作画面を開きます。

5. **動作確認**
   - フロントエンド画面から操作、またはAPI経由でコマンドを送信すると、
     BLE経由でESP32にデータが送られ、ロボットアームが動作します。

---