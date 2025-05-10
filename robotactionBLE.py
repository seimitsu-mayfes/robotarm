import asyncio
import sys
import json
import time
from bleak import BleakClient, BleakScanner
import numpy as np

# BLE設定
BLE_DEVICE_NAME = "ESP32 BLE Device"
BLE_CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
DELAY_BETWEEN_STEPS = 0.02  # 秒
MINIMUM_STEP = 10  # 角度の最小ステップ（度）

# 初期位置
INIT_POSITION = [135, 200, 30, 45, 90, 90]

# グローバルなBLEクライアント・デバイス
ble_client = None
ble_device = None

def interpolate_angles(start_angles, end_angles, min_step):
    max_diff = max([abs(end - start) for start, end in zip(start_angles, end_angles)])
    steps = max(1, int(np.ceil(max_diff / min_step)))
    angle_steps = []
    for i in range(steps + 1):
        t = i / steps
        interpolated = [int(start + (end - start) * t) for start, end in zip(start_angles, end_angles)]
        angle_steps.append(interpolated)
    return angle_steps

def generate_full_sequence(action_sequence):
    # 初期位置→動作→初期位置
    full_seq = []
    # 初期位置→動作の最初
    full_seq.extend(interpolate_angles(INIT_POSITION, action_sequence[0], MINIMUM_STEP)[1:])
    # 動作本体
    for i in range(len(action_sequence) - 1):
        full_seq.extend(interpolate_angles(action_sequence[i], action_sequence[i+1], MINIMUM_STEP)[1:])
    # 動作の最後→初期位置
    full_seq.extend(interpolate_angles(action_sequence[-1], INIT_POSITION, MINIMUM_STEP)[1:])
    # 先頭に初期位置
    return [INIT_POSITION] + full_seq

async def send_sequence_ble(client, sequence):
    for i, angles in enumerate(sequence):
        angle_str = ' '.join(map(str, angles))
        await client.write_gatt_char(BLE_CHARACTERISTIC_UUID, angle_str.encode('utf-8'))
        print(f"送信: {angle_str}")
        await asyncio.sleep(DELAY_BETWEEN_STEPS)
    print("シーケンス送信完了")

async def send_action(action_name: str):
    global ble_client, ble_device
    # robotaction.jsonからsequenceを取得
    with open("robotaction.json", "r") as f:
        action_data = json.load(f)
    if action_name not in action_data:
        raise ValueError(f"未定義のactionです: {action_name}")
    action_sequence = action_data[action_name]["sequence"]
    full_sequence = generate_full_sequence(action_sequence)
    # BLE接続維持・再接続ロジック
    print(f"[BLE] 現在の接続状態: ble_client={ble_client}, is_connected={getattr(ble_client, 'is_connected', False)}")
    if ble_client is None or not getattr(ble_client, 'is_connected', False):
        print(f"[BLE] 未接続または切断状態。再接続を試みます。")
        print(f"BLEデバイス '{BLE_DEVICE_NAME}' を検索中...")
        if ble_device is None:
            ble_device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME)
            if not ble_device:
                raise RuntimeError(f"BLEデバイスが見つかりません: {BLE_DEVICE_NAME}")
        ble_client = BleakClient(ble_device)
        await ble_client.connect()
        print(f"{ble_device.name} に再接続しました")
    else:
        print(f"[BLE] 既に接続済み。再利用します。")
    await send_sequence_ble(ble_client, full_sequence)
    await ble_client.disconnect()
    ble_client = None
    ble_device = None
    return f"{action_name} のシーケンス送信完了"

# 旧mainループはコメントアウトまたは削除
# if __name__ == "__main__":
#     asyncio.run(main()) 

if __name__ == "__main__":
    async def console():
        global ble_client, ble_device
        print("==== robotactionBLE console ====")
        print("アクション名を入力してください（qで終了）")
        while True:
            action = input("action> ").strip()
            if action.lower() == 'q':
                # 切断処理
                if ble_client and getattr(ble_client, 'is_connected', False):
                    await ble_client.disconnect()
                    print("BLE切断しました")
                print("終了します")
                break
            if not action:
                continue
            try:
                result = await send_action(action)
                print(result)
            except Exception as e:
                print(f"エラー: {e}")
                # BLE状態をリセット
                ble_client = None
                ble_device = None
    asyncio.run(console()) 