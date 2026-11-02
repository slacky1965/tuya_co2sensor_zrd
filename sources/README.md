# Source snapshots for the validated firmware

These snapshots preserve the separate source trees used for the actual
device-tested binaries. No build-time model selector or new shared DP map
is introduced in this change.

- yvx5lh6k-v1.0.07.tar.gz: YVX source commit d9e0d47; CO2 DP 0x02,
  formaldehyde DP 0x16. Tested on Bathroom AIQ and Jean's FlameAir.
- mja3fuja-v1.0.38.tar.gz: MJA source commit 72a44c9; CO2 DP 0x16,
  formaldehyde DP 0x02. Tested on Attic AIQ.

The top-level source retains upstream's existing MJA-oriented configuration.
Use the appropriate snapshot to reproduce each model's image.

Both use VOC DP 0x15, temperature DP 0x12, and humidity DP 0x13.
Both retain the existing R02 Zigbee identity, which alone does not distinguish
the original hardware. Select the binary by the original hardware signature.

Zigbee2MQTT companion precision correction:
https://github.com/Koenkk/zigbee-herdsman-converters/pull/13669

Validated OTA SHA256:

YVX 6565-0393-10073001-tuya_co2sensor_zrd.zigbee
b48726689d47fb5d666df624bd76d441ae7d879a864e3bf3e1d8e8bfdeaad7a0

MJA 6565-0393-10263001-tuya_co2sensor_zrd.zigbee
fa2d842b02baa3fa0c9eb6df6d94926875f0d9849ba5cb9164704a55ed98c62b
