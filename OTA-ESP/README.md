# ESP32 OTA Gateway for MSPM0 Programming

A fully OTA (Over-The-Air) solution for programming MSPM0 devices using ESP32 as a gateway. This system uses PlatformIO's built-in OTA SPIFFS upload system for complete wireless operation.

## 🚀 Features

- **Fully OTA**: No USB required after initial setup
- **PlatformIO Integration**: Uses built-in OTA SPIFFS upload
- **Automatic Programming**: Programs MSPM0 when new firmware detected
- **Light Sleep**: Power-efficient operation
- **BSL Protocol**: TI MSPM0 Bootloader support

## 📋 Requirements

- ESP32 (Arduino Nano ESP32 recommended)
- MSPM0G3507 target device
- WiFi network
- PlatformIO

## 🔧 Hardware Setup

### ESP32 Connections:
```
ESP32 Pin    MSPM0 Pin    Function
D12 (PA18)   PA18         BSL invoke
D11 (NRST)   NRST         Reset
D0 (TX2)     PA10         UART TX
D1 (RX2)     PA11         UART RX
D10          -             External trigger (optional)
LED_BUILTIN  -             Status LED
```

### UART Configuration:
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1

## 📦 Installation

### 1. Initial Setup (One-time)
```bash
cd OTA-ESP

# Configure ESP32 IP in platformio.ini
# Edit line: upload_port = 192.168.1.100

# Upload ESP32 code
pio run --target upload
```

### 2. Prepare MSPM0 Firmware
```bash
# Compile your MSPM0 firmware
arm-none-eabi-objcopy -O binary your_app.out mspm0_firmware.bin

# Copy to data folder
cp mspm0_firmware.bin OTA-ESP/data/
```

### 3. Upload Firmware OTA
```bash
# Upload MSPM0 firmware to ESP32 SPIFFS
pio run -t uploadfs --upload-port 192.168.1.100
```

## 🔄 Workflow

### For Developers:
1. **Compile MSPM0 firmware** → `mspm0_firmware.bin`
2. **Place in data/ folder**
3. **Upload OTA** → `pio run -t uploadfs --upload-port <ESP_IP>`
4. **ESP32 automatically programs** MSPM0

### For End Users:
1. **Someone uploads firmware** via PlatformIO OTA
2. **ESP32 automatically detects** the new file
3. **ESP32 automatically programs** MSPM0
4. **LED indicates** success/failure

## 💾 SPIFFS System

### File Structure:
```
ESP32 SPIFFS:
└── /mspm0_firmware.bin    # MSPM0 firmware file
```

### Advantages:
- ✅ **PlatformIO integration** - Built-in OTA support
- ✅ **Large file support** - No 32KB limit
- ✅ **Persistent storage** - Survives power cycles
- ✅ **Simple workflow** - Standard PlatformIO commands

## ⚡ Power Management

### Light Sleep Mode:
- **Power**: ~0.8mA (vs ~50mA when awake)
- **Wake-up**: Every 5 seconds to check for new firmware
- **Response**: Immediate to new firmware detection

### Sleep Cycle:
```
Wake up → Check SPIFFS → Program if new firmware → Sleep (5s)
```

## 🔧 BSL Protocol

### Supported Commands:
- `0x12` - Connection
- `0x19` - Get Device ID
- `0x21` - Load Password
- `0x15` - Mass Erase
- `0x20` - Program Data
- `0x40` - Start Application

### Programming Sequence:
1. **Enter BSL** (PA18 high, NRST pulse)
2. **Connect** (0x12 command)
3. **Get ID** (0x19 command)
4. **Load Password** (0x21 command)
5. **Mass Erase** (0x15 command)
6. **Program Data** (0x20 command) - Block by block
7. **Start App** (0x40 command)

## 📊 Serial Output

### Startup:
```
ESP32 OTA Gateway - PlatformIO OTA SPIFFS
Setup complete. Waiting for firmware updates...
Upload firmware to SPIFFS with: pio run -t uploadfs --upload-port <ESP_IP>
Entering light sleep mode...
```

### Programming:
```
New firmware detected! Size: 2048 bytes
Auto-triggering programming...
=== Starting BSL Programming ===
Entering BSL mode...
Programming firmware from SPIFFS...
Programming 2048 bytes
Programmed 2048 bytes
=== BSL Programming Completed Successfully ===
OTA Programming completed successfully!
```

## 🛠️ Troubleshooting

### Common Issues:

#### 1. ESP32 IP Address
```bash
# Check ESP32 IP in serial monitor
pio device monitor

# Update platformio.ini with correct IP
upload_port = 192.168.1.100
```

#### 2. Firmware File Not Found
```bash
# Ensure file is named correctly
mv your_firmware.bin data/mspm0_firmware.bin

# Upload to SPIFFS
pio run -t uploadfs --upload-port <ESP_IP>
```

#### 3. BSL Programming Failed
```
Check connections:
- PA18 (BSL invoke)
- NRST (Reset)
- UART TX/RX
```

#### 4. OTA Upload Failed
```bash
# Check WiFi connection
# Verify ESP32 IP address
# Try: pio run -t uploadfs --upload-port <ESP_IP>
```

## 📁 File Structure

```
OTA-ESP/
├── src/
│   └── main.cpp              # Main ESP32 code
├── data/
│   └── mspm0_firmware.bin    # Place MSPM0 firmware here
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## 🔄 PlatformIO Commands

```bash
# Build project
pio run

# Upload ESP32 code (one-time)
pio run --target upload

# Upload MSPM0 firmware OTA
pio run -t uploadfs --upload-port 192.168.1.100

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean
```

## 📝 Configuration

### platformio.ini:
```ini
[env:arduino_nano_esp32]
platform = espressif32
board = arduino_nano_esp32
framework = arduino
board_build.filesystem = spiffs
upload_protocol = espota
upload_port = 192.168.1.100  ; Change to your ESP32 IP
```

## 🔄 Alternative: C Declaration Method

If you prefer static firmware compilation:

1. **Use the Python script** to convert .bin to C array
2. **Include in code** and uncomment C declaration lines
3. **Compile firmware** into ESP32 code

## 📝 License

Open source - feel free to modify and distribute.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

**Happy OTA Programming!** 🚀 