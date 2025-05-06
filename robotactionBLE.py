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

async def main():
    # robotaction.jsonから全actionをロード
    with open("robotaction.json", "r") as f:
        action_data = json.load(f)

    print(f"BLEデバイス '{BLE_DEVICE_NAME}' を検索中...")
    device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME)
    if not device:
        print(f"エラー: '{BLE_DEVICE_NAME}' が見つかりません")
        return
    print(f"デバイスが見つかりました。接続中...")
    try:
        async with BleakClient(device) as client:
            print(f"{device.name} に接続しました")
            print("action名を入力してください（例: greeting, nod, ...）。exit/quitで終了します。")
            while True:
                action = input("action> ").strip()
                if action.lower() in ("exit", "quit"):
                    print("終了します。")
                    break
                if action not in action_data:
                    print(f"未定義のactionです: {action}")
                    continue
                action_sequence = action_data[action]["sequence"]
                full_sequence = generate_full_sequence(action_sequence)
                print(f"{action} のシーケンスを送信します")
                await send_sequence_ble(client, full_sequence)
    except Exception as e:
        print(f"BLE接続エラー: {e}")

if __name__ == "__main__":
    asyncio.run(main()) 