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