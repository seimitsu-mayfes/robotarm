from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import asyncio
from robotactionBLE import send_action, ble_client, ble_device, BLE_DEVICE_NAME
from bleak import BleakClient, BleakScanner

app = FastAPI()

class ActionRequest(BaseModel):
    action: str

@app.on_event("startup")
async def startup_event():
    global ble_client, ble_device
    try:
        ble_device = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME)
        if not ble_device:
            print(f"警告: BLEデバイスが見つかりません: {BLE_DEVICE_NAME}。リクエスト時に再スキャンします。")
            ble_device = None
            ble_client = None
        else:
            ble_client = BleakClient(ble_device)
            await ble_client.connect()
            print("BLE接続確立")
    except Exception as e:
        print(f"BLE初期化時に例外: {e}。リクエスト時に再スキャンします。")
        ble_device = None
        ble_client = None

@app.on_event("shutdown")
async def shutdown_event():
    global ble_client
    if ble_client and ble_client.is_connected:
        await ble_client.disconnect()
        print("BLE切断")

@app.post("/action")
async def do_action(req: ActionRequest):
    try:
        print(f"[API] POST /action 受信: action={req.action}")
        result = await send_action(req.action)
        return {"status": "ok", "detail": result}
    except Exception as e:
        print(f"[API] /action エラー: {e}")
        raise HTTPException(status_code=500, detail=str(e)) 