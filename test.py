import requests
import time
import random
import json

# 🔧 CONFIG
URL = "http://localhost/api/controller_update.php"
DEVICE_ID = "001a2b3c4d5e"

# requests.packages.urllib3.disable_warnings()

# 📊 Stats
total_tx = 0
total_rx = 0
request_count = 0

while True:
    try:
        start_time = time.time()

        # 🎲 Generate random values
        water_level = random.randint(0, 100)
        voltage = random.randint(200, 260)
        motor_status = request_count % 5 == 0 
        payload = {
            "id": DEVICE_ID,
            "wl": water_level,
            "volt": voltage,
            "mt": motor_status
        }

        # Convert to JSON manually (so we can measure size)
        payload_str = json.dumps(payload)
        tx_bytes = len(payload_str.encode("utf-8"))

        response = requests.post(
            URL,
            data=payload_str,
            headers={"Content-Type": "application/json"},
            timeout=5
        )

        latency = (time.time() - start_time) * 1000  # ms

        # Measure received bytes
        rx_bytes = len(response.content)

        # Update stats
        total_tx += tx_bytes
        total_rx += rx_bytes
        request_count += 1

        print("\n==============================")
        print(f"📡 Request #{request_count}")
        print(f"⏱️ Latency: {latency:.2f} ms")
        print(f"⬆️ TX (sent): {tx_bytes} bytes")
        print(f"⬇️ RX (received): {rx_bytes} bytes")

        print(f"📊 Total TX: {total_tx} bytes")
        print(f"📊 Total RX: {total_rx} bytes")

        if response.status_code == 200:
            try:
                data = response.json()

                if data.get("s") == 1 and data.get("nd") == 1:
                    print("🔔 NEW CONFIG RECEIVED")
                    print("Config:", data["cfg"])
                else:
                    print("ℹ️ No new config")
            except:
                print("⚠️ JSON parse error:", response.text)
        else:
            res = response.json()
            print("❌ HTTP Error:", response.status_code, "- Response:", res.get("m", "No message"))

    except Exception as e:
        print("🚫 Request failed:", str(e))

    time.sleep(10)