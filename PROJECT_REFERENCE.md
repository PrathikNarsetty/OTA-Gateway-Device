# OTA Gateway Project Reference

## Quick Reference for AI Assistant

### Project Overview
- **ESP32**: Host MCU controlling MSPM0G3507 programming
- **MSPM0G3507**: Target device receiving firmware updates
- **Communication**: UART with TI MSPM0 Bootloader (BSL) protocol

### Key Hardware Connections
- ESP32 D12 → MSPM0G3507 PA18 (Boot Mode)
- ESP32 D11 → MSPM0G3507 NRST (Reset)
- ESP32 UART TX/RX → MSPM0G3507 UART RX/TX
- Common GND

### Critical Timing (from your current code)
```cpp
// Reset sequence timing:
digitalWrite(D12, LOW);    // PA18 LOW
digitalWrite(D11, LOW);    // NRST LOW (reset)
delay(10);                 // Hold reset for 10ms
digitalWrite(D12, HIGH);   // PA18 HIGH (before NRST release)
delay(1);                  // 1ms delay to ensure latching
digitalWrite(D11, HIGH);   // Release NRST
delay(10);                 // Hold PA18 high for 10ms
```

### BSL Protocol Commands
- **0x07**: BSL_TX_BSL_VERSION - Get bootloader version
- **0x03**: BSL_MASS_ERASE - Erase flash memory
- **0x00**: BSL_RX_DATA_BLOCK - Receive firmware data
- **0x06**: BSL_TX_DATA_BLOCK - Transmit data (verification)

### Current Implementation Status
- ✅ GPIO control for reset and boot mode
- ✅ Basic reset sequence implemented
- ❌ UART communication with BSL
- ❌ Firmware programming logic
- ❌ Error handling and verification

### Development Environment
- **ESP32**: PlatformIO with Arduino framework
- **MSPM0G3507**: TI Code Composer Studio with TI-CLANG

### Reference Documents Available
- `bsl.pdf` - TI MSPM0 Bootloader documentation
- `ArduinoNanoESP32Datasheet.pdf` - ESP32 datasheet

### Next Steps Needed
1. Implement UART communication (Serial2)
2. Add BSL command functions
3. Implement firmware programming sequence
4. Add error handling and verification
5. Test complete programming cycle 