import requests
import hashlib
import json
import base64
import time

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from cryptography.hazmat.backends import default_backend

# ================= CONFIG =================
ESP32_URL = "http://10.174.113.64:80/update"   # CHANGE THIS
FIRMWARE_PATH = ".\\WLMS_MASTER\\wlms_master.ino\\build\\esp32.esp32.esp32\\wlms_master.ino.ino.bin"
PRIVATE_KEY_PATH = "private.pem"

DEVICE_ID = "esp32_001"
VERSION = "1.0.2"

# ================= LOAD FIRMWARE =================
with open(FIRMWARE_PATH, "rb") as f:
    firmware_data = f.read()

# ================= HASH =================
firmware_hash = hashlib.sha256(firmware_data).hexdigest()

# ================= METADATA =================
metadata_dict = {
    "device_id": DEVICE_ID,
    "version": VERSION,
    "hash": firmware_hash,
    "timestamp": int(time.time())
}

# IMPORTANT: compact JSON (NO spaces)
metadata = json.dumps(metadata_dict, separators=(',', ':'))

print("Metadata:")
print(metadata)

# ================= LOAD PRIVATE KEY =================
with open(PRIVATE_KEY_PATH, "rb") as f:
    private_key = serialization.load_pem_private_key(
        f.read(),
        password=None,
        backend=default_backend()
    )

# ================= SIGN =================
signature = private_key.sign(
    data=metadata.encode(),
    signature_algorithm=ec.ECDSA(hashes.SHA256())
)

# Convert signature to base64 (DER format already correct)
signature_b64 = base64.b64encode(signature).decode()

print("\nSignature (base64):")
print(signature_b64)

# ================= SEND REQUEST =================
files = {
    "update": ("update.bin", firmware_data, "application/octet-stream")
}

data = {
    "metadata": metadata,
    "signature": signature_b64
}

print("\nUploading...")

response = requests.post(ESP32_URL, files=files, data=data)

print("\nResponse:")
print(response.status_code)
print(response.text)