# ESP32 OTA Gateway for MSPM0 Programming - Sleep Mode

A power-efficient ESP32 application that programs MSPM0 devices using TI's Bootloader (BSL) protocol. The ESP32 stays in deep sleep mode until triggered, then programs the MSPM0 and returns to sleep.

## Features

- **Ultra-Low Power**: ESP32 stays in deep sleep mode (consumes ~10µA)
- **File-Based Programming**: Reads firmware from SPIFFS filesystem
- **Complete BSL Protocol**: Full MSPM0 bootloader implementation
- **External Trigger**: Wake up via GPIO signal
- **Status Indication**: LED shows programming status
- **Error Handling**: Comprehensive error checking and recovery

## Hardware Requirements

### ESP32 (Arduino Nano ESP32)
- **D12** → MSPM0G3507 **PA18** (BSL invoke pin)
- **D11** → MSPM0G3507 **NRST** (Reset pin)
- **D0** → MSPM0G3507 **PA11** (UART TX)
- **D1** → MSPM0G3507 **PA10** (UART RX)
- **D10** → External trigger (connect to GND to wake)
- **GND** → Common ground

### MSPM0G3507
- Must have BSL bootloader enabled
- UART communication at 9600 baud
- 8-bit data, 1 stop bit, no parity

## Setup Instructions

### 1. Prepare Firmware File

1. **Compile your MSPM0 firmware** to `.bin` or `.hex` format
2. **Place the file** in the `data/` folder with the name `firmware.bin`
3. **Verify file size** fits within MSPM0 flash memory

### 2. Build and Upload

```bash
# Upload the ESP32 application
pio run --target upload

# Upload the firmware file to SPIFFS
pio run --target uploadfs

# Monitor serial output
pio device monitor
```

### 3. Hardware Setup

1. **Connect ESP32 to MSPM0** using the specified pins
2. **Connect trigger signal** to D10 (optional - for external wake)
3. **Power up the system**

## Operation

### Normal Operation (Sleep Mode)
- ESP32 enters deep sleep immediately after boot
- LED is OFF to indicate sleep mode
- Power consumption: ~10µA
- Serial monitor shows: "Entering deep sleep mode..."

### OTA Trigger Methods

#### Method 1: External Signal
- Connect D10 to GND to wake the ESP32
- ESP32 will immediately start programming

#### Method 2: Manual Reset
- Press the reset button on ESP32
- ESP32 will check for firmware and program if available

### Programming Sequence

When triggered, the ESP32 will:

1. **Wake up** and initialize systems
2. **Check for firmware file** in SPIFFS
3. **Enter BSL mode** on MSPM0
4. **Establish connection** with bootloader
5. **Get device ID** for verification
6. **Load password** (unlocked device)
7. **Mass erase** flash memory
8. **Program firmware** in 128-byte blocks
9. **Start application** on MSPM0
10. **Return to sleep** mode

### Status Indicators

- **LED OFF**: Sleep mode or programming failed
- **LED ON**: Programming in progress or successful completion
- **Serial Output**: Detailed status and progress information

## File Structure

```
OTA-ESP/
├── src/
│   ├── main.cpp          # Main application code
│   └── mainSoftwareInvoke.cpp  # Alternative BSL implementation
├── data/
│   ├── README.txt        # Instructions for firmware files
│   └── firmware.bin      # Your MSPM0 firmware (place here)
├── platformio.ini        # PlatformIO configuration
└── README.md            # This file
```

## BSL Programming Details

### Protocol Commands Used
- **0x12**: BSL Connection
- **0x19**: Get Device ID
- **0x21**: Load Password
- **0x15**: Mass Erase
- **0x20**: Program Data Block
- **0x40**: Start Application

### Timing Sequence
```
1. PA18 HIGH (500ms)
2. NRST LOW (2ms)
3. NRST HIGH
4. PA18 HIGH (500ms)
5. BSL Communication
```

## Troubleshooting

### Common Issues

1. **No Firmware File Found**
   - Ensure `firmware.bin` is in the `data/` folder
   - Upload SPIFFS data: `pio run --target uploadfs`

2. **BSL Communication Failed**
   - Check UART connections (D0→PA11, D1→PA10)
   - Verify MSPM0 has BSL enabled
   - Check timing of reset sequence

3. **Programming Fails**
   - Verify firmware file format and size
   - Check MSPM0 is not write-protected
   - Monitor serial output for error details

4. **ESP32 Won't Wake**
   - Check trigger signal on D10
   - Verify GPIO configuration
   - Try manual reset

### Debug Information

Enable detailed logging by monitoring serial output:
```bash
pio device monitor
```

## Power Consumption

- **Sleep Mode**: ~10µA
- **Active Programming**: ~100mA
- **Idle Mode**: ~50mA

## Customization

### Changing Trigger Pin
Edit in `src/main.cpp`:
```cpp
#define PIN_TRIGGER D10   // Change to desired pin
```

### Changing Firmware Path
Edit in `src/main.cpp`:
```cpp
const char* FIRMWARE_PATH = "/firmware.bin";  // Change filename
```

### Adding Multiple Firmware Files
Modify the code to support multiple firmware files:
```cpp
// Add firmware selection logic
const char* FIRMWARE_PATHS[] = {"/firmware1.bin", "/firmware2.bin"};
```

## PlatformIO Commands

```bash
# Build project
pio run

# Upload firmware
pio run --target upload

# Upload SPIFFS data
pio run --target uploadfs

# Monitor serial
pio device monitor

# Clean build
pio run --target clean
```

## Security Considerations

- Firmware files are stored in SPIFFS (readable)
- No authentication required for programming
- Consider adding firmware validation
- Implement checksum verification

## License

This project is provided as-is for educational and development purposes.

## Contributing

Feel free to submit issues and enhancement requests! 