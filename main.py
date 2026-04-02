import requests
import time
import hmac
import hashlib
import json
import secrets

# 🔐 CONFIG
ESP_URL = "http://10.174.113.64/api/7f3a9c_motor_ctrl"
SECRET = bytes.fromhex("3e3c9fe7e5d099e1013e8f20c52b46ff0cea526d7bf472fc3195a928284300ce")

# ⚙️ Generate payload
def create_payload(action):
    payload = {
        "action": action,
        "timestamp": int(time.time()),
        "nonce": secrets.token_hex(8)  # 16 hex chars
    }
    return payload

# 🔏 Sign payload
def sign_payload(payload):
    message = json.dumps(payload, separators=(',', ':'))  # IMPORTANT: no spaces
    signature = hmac.new(SECRET, message.encode(), hashlib.sha256).hexdigest()
    return signature, message

# 🚀 Send request
def send_command(action):
    payload = create_payload(action)
    sig, message = sign_payload(payload)

    payload["sig"] = sig

    print("Sending payload:")
    print(json.dumps(payload, indent=2))

    try:
        response = requests.post(
            ESP_URL,
            json=payload,
            timeout=3
        )

        print("\nResponse:")
        print("Status:", response.status_code)
        print("Body:", response.text)

    except requests.exceptions.RequestException as e:
        print("Request failed:", e)

# ▶️ TEST
if __name__ == "__main__":
    send_command("motor_on")
    # send_command("motor_off")