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