"""Verify model-separated OTA headers and embedded hardware signatures."""
from pathlib import Path
import hashlib
import struct

root = Path(__file__).resolve().parents[1]
for model, version, other in (("yvx5lh6k", 0x10073001, "mja3fuja"), ("mja3fuja", 0x10263001, "yvx5lh6k")):
    path = root / "bin" / model / f"6565-0393-{version:08x}-tuya_co2sensor_zrd.zigbee"
    data = path.read_bytes()
    assert struct.unpack_from("<I", data)[0] == 0x0BEEF11E
    assert struct.unpack_from("<I", data, 14)[0] == version
    assert struct.unpack_from("<I", data, 52)[0] == len(data)
    assert model.encode() in data
    assert other.encode() not in data
    print(model, hex(version), hashlib.sha256(data).hexdigest())
