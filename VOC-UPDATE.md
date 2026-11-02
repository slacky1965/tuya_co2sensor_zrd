# Model-specific secondary firmware

This update corrects VOC scaling for the two tested R02 hardware variants.
DP 0x15 is ppb. The firmware reports raw / 1e9; the existing R02
Zigbee2MQTT definition scales that to raw / 1000 ppm.

| Original signature | Build command | Version | OTA header |
| --- | --- | --- | --- |
| _TZE200_yvx5lh6k | make SENSOR_MODEL=yvx5lh6k | 1.0.07 | 0x10073001 |
| _TZE200_mja3fuja | make SENSOR_MODEL=mja3fuja | 1.0.38 | 0x10263001 |

Run make clean before switching targets. Keep images in separate model
directories. Both retain Tuya_CO2Sensor_r02 for existing Z2M support;
that shared identity is not sufficient to choose the hardware image.

Already migrated devices use direct secondary OTA. Do not repeat the
first-stage bootloader migration. Devices with higher historical custom
file versions require the explicit OTA downgrade flow.

Zigbee2MQTT also needs three-decimal precision in both the R02 converter
and final VOC post-processing. Until that correction is available,
voc_precision: 3 alone does not bypass the converter's earlier two-decimal
rounding. This firmware update does not measure or improve gas calibration.

The previous separately built YVX image completed OTA on Bathroom and
Jean's FlameAir, and the MJA image completed OTA on Attic. The consolidated
build targets must be checked separately; do not claim they were flashed
merely because the earlier artifacts worked.
