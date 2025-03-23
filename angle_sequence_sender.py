import asyncio
import sys
import time
import json
import numpy as np
import os
from bleak import BleakClient, BleakScanner

# 設定パラメータ
CONFIG = {
    "angle_file": "angle_sequences.json",  # 制御角のセットが保存されているファイル
    "interpolated_file": "interpolated_angles.json",  # 補間された角度を保存するファイル
    "minimum_step": 10,  # 角度の最小ステップ（度）
    "delay_between_steps": 0.02,  # ステップ間の遅延（秒）
    "delay_between_sequences": 1.0,  # シーケンス間の遅延（秒）
    "repeat_count": 3,  # シーケンスの繰り返し回数（-1で無限ループ）
    "ble_device_name": "ESP32 BLE Device",  # BLEデバイス名
    "ble_characteristic_uuid": "beb5483e-36e1-4688-b7f5-ea07361b26a8"  # BLEキャラクタリスティックUUID
}

# BLE通知ハンドラ
def notification_handler(sender, data):
    """BLEデバイスからの通知を処理する"""
    print(f"BLEデバイスからの通知: {data.decode()}")

def load_angle_sequences(filename):
    """JSONファイルから角度シーケンスを読み込む"""
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
        print(f"角度シーケンスを読み込みました: {filename}")
        return data
    except Exception as e:
        print(f"エラー: 角度シーケンスファイルの読み込みに失敗しました: {e}")
        sys.exit(1)

def interpolate_angles(start_angles, end_angles, min_step):
    """2つの角度セット間を最小ステップで分割する"""
    # 各サーボの角度差の最大値を計算
    max_diff = max([abs(end - start) for start, end in zip(start_angles, end_angles)])
    
    # 必要なステップ数を計算（最小ステップで分割）
    steps = max(1, int(np.ceil(max_diff / min_step)))
    
    # 各ステップの角度を計算
    angle_steps = []
    for i in range(steps + 1):
        # 線形補間
        t = i / steps
        interpolated = [int(start + (end - start) * t) for start, end in zip(start_angles, end_angles)]
        angle_steps.append(interpolated)
    
    return angle_steps

def generate_all_interpolated_angles():
    """すべての角度シーケンスを補間して別ファイルに保存する"""
    # 角度シーケンスを読み込む
    sequences = load_angle_sequences(CONFIG["angle_file"])
    
    # 補間結果を格納する辞書
    all_interpolated = {}
    
    # 各シーケンスを処理
    for name, sequence in sequences.items():
        print(f"シーケンス '{name}' を補間中...")
        
        # このシーケンスの補間結果
        interpolated_sequence = []
        
        # 前の角度セットを保存（最初はNone）
        prev_angles = None
        
        for i, angles in enumerate(sequence):
            # 前の角度セットがある場合は補間
            if prev_angles is not None:
                interpolated_steps = interpolate_angles(prev_angles, angles, CONFIG["minimum_step"])
                # 最初のステップは前の角度と同じなのでスキップ
                interpolated_sequence.extend(interpolated_steps[1:])
            else:
                # 最初の角度セットはそのまま追加
                interpolated_sequence.append(angles)
            
            # 現在の角度を保存
            prev_angles = angles
        
        # この補間シーケンスを結果に追加
        all_interpolated[name] = interpolated_sequence
    
    # 補間結果をファイルに保存
    with open(CONFIG["interpolated_file"], 'w') as f:
        json.dump(all_interpolated, f, indent=2)
    
    print(f"補間された角度シーケンスを保存しました: {CONFIG['interpolated_file']}")
    
    return all_interpolated

async def execute_interpolated_angles():
    """補間された角度シーケンスを実行する"""
    # 補間された角度シーケンスを読み込む
    try:
        with open(CONFIG["interpolated_file"], 'r') as f:
            all_interpolated = json.load(f)
    except Exception as e:
        print(f"エラー: 補間された角度ファイルの読み込みに失敗しました: {e}")
        return
    
    print("角度シーケンス送信プログラムを開始します")
    
    # BLEデバイスを検索
    print(f"'{CONFIG['ble_device_name']}'を検索中...")
    device = await BleakScanner.find_device_by_name(CONFIG["ble_device_name"])
    
    if not device:
        print(f"エラー: '{CONFIG['ble_device_name']}'が見つかりません")
        return
    
    print(f"デバイスが見つかりました。接続中...")
    
    # BLEデバイスに接続
    try:
        async with BleakClient(device) as client:
            print(f"{device.name}に接続しました")
            
            # 通知を受信するためのハンドラを設定
            await client.start_notify(CONFIG["ble_characteristic_uuid"], notification_handler)
            
            # 繰り返し回数
            repeat = CONFIG["repeat_count"]
            count = 0
            
            while repeat == -1 or count < repeat:
                if repeat != -1:
                    count += 1
                    print(f"\n*** 繰り返し {count}/{repeat} ***")
                
                # 各シーケンスを処理
                for name, interpolated_sequence in all_interpolated.items():
                    print(f"\n=== シーケンス '{name}' を開始 ({len(interpolated_sequence)}ステップ) ===")
                    
                    for i, angles in enumerate(interpolated_sequence):
                        if i % 10 == 0 or i == len(interpolated_sequence) - 1:  # 10ステップごとに進捗表示
                            print(f"ステップ {i+1}/{len(interpolated_sequence)}")
                        
                        # 角度を文字列に変換
                        angle_str = ' '.join(map(str, angles))
                        
                        try:
                            # BLEでデータを送信
                            await client.write_gatt_char(CONFIG["ble_characteristic_uuid"], angle_str.encode('utf-8'))
                            print(f"送信: {angle_str}")
                        except Exception as e:
                            print(f"送信エラー: {e}")
                            # エラーが発生した場合はファイルに保存
                            with open("failed_angles.txt", "a") as f:
                                f.write(f"{angle_str}\n")
                            print(f"送信に失敗した角度をfailed_angles.txtに保存しました")
                        
                        # 次のステップまで待機
                        await asyncio.sleep(CONFIG["delay_between_steps"])
                    
                    print(f"=== シーケンス '{name}' 完了 ===\n")
                    await asyncio.sleep(CONFIG["delay_between_sequences"])
            
            # 通知の受信を停止
            await client.stop_notify(CONFIG["ble_characteristic_uuid"])
            
    except Exception as e:
        print(f"BLE接続エラー: {e}")
        print("プログラムを終了します")
    
    print("BLEデバイスから切断しました")

async def main():
    """メイン関数"""
    # 設定ファイルの存在確認と作成
    check_and_create_sample_file()
    
    # 補間された角度を生成して保存
    all_interpolated = generate_all_interpolated_angles()
    
    # 補間結果の概要を表示
    print("\n補間結果の概要:")
    for name, sequence in all_interpolated.items():
        print(f"- {name}: {len(sequence)}ステップ")
    
    # ユーザーに実行の確認を求める
    while True:
        response = input("\n補間された角度シーケンスを実行しますか？ (y/n): ").lower()
        if response == 'y':
            await execute_interpolated_angles()
            break
        elif response == 'n':
            print("プログラムを終了します")
            break
        else:
            print("'y'または'n'を入力してください")

def check_and_create_sample_file():
    """サンプルファイルの存在確認と作成"""
    if not os.path.exists(CONFIG["angle_file"]):
        # サンプルの角度シーケンスファイルを作成
        sample_sequences = {
            "初期位置に戻る": [
                [135, 200, 30, 45, 90, 90]
            ],
            "手を振る": [
                [135, 200, 30, 45, 90, 90],
                [135, 200, 30, 45, 45, 90],
                [135, 200, 30, 45, 135, 90],
                [135, 200, 30, 45, 45, 90],
                [135, 200, 30, 45, 135, 90],
                [135, 200, 30, 45, 90, 90]
            ],
            "お辞儀": [
                [135, 200, 30, 45, 90, 90],
                [135, 150, 80, 45, 90, 90],
                [135, 200, 30, 45, 90, 90]
            ]
        }
        
        with open(CONFIG["angle_file"], 'w') as f:
            json.dump(sample_sequences, f, indent=2)
        
        print(f"サンプルの角度シーケンスファイルを作成しました: {CONFIG['angle_file']}")
        print("このファイルを編集して、独自の角度シーケンスを定義できます。")

if __name__ == "__main__":
    # メイン関数を実行
    asyncio.run(main()) 