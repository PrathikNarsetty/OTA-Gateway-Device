OTA Gateway Firmware Storage

This folder is for storing firmware files that will be programmed to MSPM0 devices.

INSTRUCTIONS:
1. Place your MSPM0 firmware file here with the name "firmware.bin"
2. The file should be in binary format (.bin) or Intel HEX format (.hex)
3. Make sure the file size fits within the MSPM0 flash memory
4. Upload this folder to the ESP32 using PlatformIO SPIFFS upload

PLATFORMIO COMMANDS:
- Upload firmware: pio run --target upload
- Upload SPIFFS data: pio run --target uploadfs
- Monitor serial: pio device monitor

FILE FORMATS SUPPORTED:
- .bin (binary firmware)
- .hex (Intel HEX format)

EXAMPLE:
- firmware.bin (your MSPM0 firmware file)
- firmware.hex (alternative format)

The ESP32 will automatically look for "firmware.bin" in this location. 