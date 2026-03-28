from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
import json, base64

# Load private key
with open("private.pem", "rb") as f:
    private_key = serialization.load_pem_private_key(f.read(), password=None)

# Metadata
metadata = {
    "version": "1.0.1",
    "size": 1048576,
    "hash": "abc123...",
    "timestamp": 1710000000
}

data = json.dumps(metadata, separators=(',', ':')).encode()

# Sign
signature = private_key.sign(data, ec.ECDSA(hashes.SHA256()))

print("SIGNATURE (base64):", base64.b64encode(signature).decode())
print("METADATA:", data.decode())