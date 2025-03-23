import asyncio
from bleak import BleakClient, BleakScanner
import aioconsole

# ESP32のBLEデバイス名
DEVICE_NAME = "ESP32 BLE Device"  # 接続するデバイスの名前

# UUIDs
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  # サービスのUUID
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"  # キャラクタリスティックのUUID

async def notification_handler(sender, data):
    """通知を処理するコールバック関数"""
    print(f"受信した通知: {data.decode()}")  # 受信した通知をデコードして表示

async def user_input(client):
    """ユーザーからの入力を処理し、BLEデバイスにコマンドを送信する関数"""
    print("6つのサーボモーターの角度を入力してください（例: '135 140 120 90 60 89'）")
    print("終了するには 'q' を入力してください")
    
    while True:
        # ユーザーからのコマンド入力を非同期で待機
        command = await aioconsole.ainput("6つの角度を入力（スペース区切り）: ")
        
        if command.lower() == 'q':
            return False  # 'q'が入力されたらループを終了
        
        # 入力を検証
        try:
            angles = command.split()
            if len(angles) != 6:
                print("エラー: 6つの角度を入力してください")
                continue
                
            # 全ての値が数値であることを確認
            for angle in angles:
                int(angle)  # 数値に変換できるか確認
                
            # コマンドをUTF-8でエンコードしてBLEデバイスに送信
            await client.write_gatt_char(CHARACTERISTIC_UUID, command.encode('utf-8'))
            print(f"送信したコマンド: {command}")  # 送信したコマンドを表示
            
        except ValueError:
            print("エラー: 全ての値は数値である必要があります")
        except Exception as e:
            print(f"エラーが発生しました: {e}")

async def main():
    """メイン関数：BLEデバイスに接続し、通知を受信する"""
    print(f"'{DEVICE_NAME}'を検索中...")
    
    # デバイス名でBLEデバイスをスキャンし、デバイスを見つける
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)
    
    if not device:
        print(f"'{DEVICE_NAME}'という名前のデバイスが見つかりませんでした")  # デバイスが見つからない場合のメッセージ
        return

    print(f"デバイスが見つかりました。接続中...")
    
    # デバイスに接続し、接続が確立されたら以下の処理を行う
    try:
        async with BleakClient(device) as client:
            print(f"{device.name}に接続しました")  # デバイスに接続したことを表示

            # 通知を受信するためのハンドラを設定
            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

            try:
                # ユーザー入力を処理してコマンドを送信
                await user_input(client)
            except asyncio.CancelledError:
                pass

            # 通知の受信を停止
            await client.stop_notify(CHARACTERISTIC_UUID)

        print("切断しました")  # デバイスから切断したことを表示
    
    except Exception as e:
        print(f"接続エラー: {e}")

if __name__ == "__main__":
    # メイン関数を実行
    asyncio.run(main()) 