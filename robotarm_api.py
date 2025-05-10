from fastapi import FastAPI, HTTPException, BackgroundTasks
from pydantic import BaseModel
import asyncio
from robotactionBLE import send_action, ble_client, ble_device, BLE_DEVICE_NAME, BLE_CHARACTERISTIC_UUID
from bleak import BleakClient, BleakScanner
from typing import Dict

app = FastAPI()

action_status: Dict[str, str] = {}  # action_id: "pending" or "done"

class ActionRequest(BaseModel):
    action: str
    action_id: str = None

# @app.on_event("startup")
# async def startup_event():
#     global ble_client, ble_device
#     try:
#         ble_device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME)
#         if not ble_device:
#             print(f"警告: BLEデバイスが見つかりません: {BLE_DEVICE_NAME}。リクエスト時に再スキャンします。")
#             ble_device = None
#             ble_client = None
#         else:
#             ble_client = BleakClient(ble_device)
#             await ble_client.connect()
#             print("BLE接続確立")
#     except Exception as e:
#         print(f"BLE初期化時に例外: {e}。リクエスト時に再スキャンします。")
#         ble_device = None
#         ble_client = None

@app.on_event("shutdown")
async def shutdown_event():
    global ble_client
    if ble_client and ble_client.is_connected:
        await ble_client.disconnect()
        print("BLE切断")

@app.post("/action")
async def do_action(req: ActionRequest, background_tasks: BackgroundTasks):
    action_id = req.action_id or (str(asyncio.get_event_loop().time()))
    action_status[action_id] = "pending"
    background_tasks.add_task(run_ble_action, req.action, action_id)
    return {"status": "started"}

async def run_ble_action(action, action_id):
    try:
        await send_action(action)
        action_status[action_id] = "done"
    except Exception as e:
        action_status[action_id] = f"error: {e}"

@app.get("/action_status")
async def get_action_status(action_id: str):
    return {"status": action_status.get(action_id, "unknown")}

async def ensure_connected():
    global ble_client, ble_device
    if ble_client is not None and getattr(ble_client, 'is_connected', False):
        try:
            # 実際にGATT通信で疎通確認
            await ble_client.read_gatt_char(BLE_CHARACTERISTIC_UUID)
            return True
        except Exception:
            print("GATT通信失敗。再接続します。")
    # ここまで来たら再接続
    ble_device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME)
    if not ble_device:
        ble_client = None
        return False
    ble_client = BleakClient(ble_device)
    await ble_client.connect()
    try:
        await ble_client.read_gatt_char(BLE_CHARACTERISTIC_UUID)
        return True
    except Exception:
        ble_client = None
        return False 