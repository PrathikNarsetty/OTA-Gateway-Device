# Power Management & Filesystem Guide

## Power Consumption Comparison

### ESP32 Power Modes

| Mode | Power Consumption | WiFi Status | Wake Time | Use Case |
|------|------------------|-------------|-----------|----------|
| **Active Mode** | 100-200mA | Connected | Instant | Programming, Web Server |
| **Light Sleep** | ~0.8mA | **Stays Active** | ~1ms | **Network-based triggers** |
| **Deep Sleep** | ~10µA | Completely Off | ~1-2s | Battery backup |

### Why Light Sleep is Better for Your Use Case

**Light Sleep Advantages:**
- ✅ **WiFi stays connected** - Can receive network packets
- ✅ **Fast wake-up** (~1ms vs ~1s for deep sleep)
- ✅ **Still saves 95% power** vs active mode
- ✅ **Can wake on network activity**
- ✅ **Maintains web server availability**

**Deep Sleep Limitations:**
- ❌ WiFi completely off - No network triggers
- ❌ Slow wake-up (~1-2 seconds)
- ❌ Requires external GPIO trigger

## SPIFFS Filesystem Explained

### What is SPIFFS?

**SPIFFS** = **SPI Flash File System**
- A filesystem that runs on ESP32's flash memory
- Similar to a small SD card built into the ESP32
- Stores files persistently across reboots
- Perfect for storing firmware files

### How the Data Folder Works

```
OTA-ESP/
├── data/                    # ← This folder gets uploaded to SPIFFS
│   ├── firmware.bin        # ← Your MSPM0 firmware file
│   └── README.txt          # ← Instructions file
├── src/
│   └── main.cpp            # ← ESP32 application code
└── platformio.ini          # ← Configuration
```

### The Upload Process

1. **User places files in `data/` folder**
   ```
   OTA-ESP/data/firmware.bin  ← Your MSPM0 firmware
   ```

2. **PlatformIO uploads data folder to SPIFFS**
   ```bash
   pio run --target uploadfs  ← Uploads data/ to ESP32 SPIFFS
   ```

3. **ESP32 code reads from SPIFFS**
   ```cpp
   File file = SPIFFS.open("/firmware.bin", "r");  ← Reads from SPIFFS
   ```

### File Paths Explained

- **Host (your computer)**: `OTA-ESP/data/firmware.bin`
- **ESP32 SPIFFS**: `/firmware.bin` (note the leading slash)

The `data/` folder contents become the root of SPIFFS on the ESP32.

## User Workflow

### Step 1: Prepare Firmware
```bash
# Compile your MSPM0 firmware to .bin format
# Place it in the data folder
cp your_mspm0_firmware.bin OTA-ESP/data/firmware.bin
```

### Step 2: Upload Everything
```bash
# Upload ESP32 application code
pio run --target upload

# Upload firmware file to SPIFFS
pio run --target uploadfs
```

### Step 3: Use the System
- **Web Interface**: Visit `http://[ESP32_IP]` to trigger programming
- **External Trigger**: Connect D10 to GND
- **Network Trigger**: Send HTTP POST to `/trigger`

## Power Savings Analysis

### Light Sleep Mode Benefits

**Power Consumption:**
- **Active Mode**: 150mA average
- **Light Sleep**: 0.8mA average
- **Power Savings**: 99.5% reduction

**Battery Life Examples:**
- **2000mAh battery with active mode**: ~13 hours
- **2000mAh battery with light sleep**: ~100 days
- **10000mAh battery with light sleep**: ~500 days

### Wake-up Triggers

**Light Sleep can wake on:**
- ✅ **WiFi packets** (network activity)
- ✅ **GPIO signals** (external triggers)
- ✅ **Timer** (periodic wake)
- ✅ **Web server requests**

**Deep Sleep can only wake on:**
- ❌ GPIO signals only
- ❌ Timer only
- ❌ No network activity possible

## Network-Based Triggering

### Web Interface
```html
<!-- User visits: http://192.168.1.100 -->
<button onclick="triggerProgramming()">Start Programming</button>
```

### HTTP API
```bash
# Trigger programming via HTTP
curl -X POST http://192.168.1.100/trigger

# Check status
curl http://192.168.1.100/status
```

### Network Wake-up
- ESP32 stays connected to WiFi in light sleep
- Can receive HTTP requests even while sleeping
- Automatically wakes to handle network activity
- Returns to sleep after processing

## Code Structure

### Light Sleep Implementation
```cpp
void enterLightSleep() {
  // Configure wake sources
  esp_sleep_enable_timer_wakeup(10000000); // 10 seconds
  esp_sleep_enable_ext0_wakeup(PIN_TRIGGER, 0);
  
  // Enter light sleep (WiFi stays active)
  esp_light_sleep_start();
  
  // Automatically wakes on network activity
}
```

### SPIFFS File Reading
```cpp
void triggerProgramming() {
  // Check if firmware exists in SPIFFS
  if (!SPIFFS.exists("/firmware.bin")) {
    Serial.println("No firmware file found!");
    return;
  }
  
  // Read firmware from SPIFFS
  File file = SPIFFS.open("/firmware.bin", "r");
  // ... program MSPM0 with file data
}
```

## Troubleshooting

### SPIFFS Issues
```bash
# Check if files uploaded correctly
pio run --target uploadfs

# Monitor serial to see file detection
pio device monitor
```

### Power Issues
- **High power consumption**: Check if WiFi is connected
- **Won't wake**: Verify wake-up sources configured
- **Network not working**: Ensure light sleep (not deep sleep)

### File Issues
- **File not found**: Ensure uploaded with `uploadfs`
- **Wrong file**: Check filename matches code (`firmware.bin`)
- **File too large**: Check ESP32 SPIFFS size limits

## Summary

**Light Sleep + SPIFFS = Perfect OTA Solution**

- **Ultra-low power** (~0.8mA) while keeping WiFi active
- **Network triggers** via web interface or HTTP API
- **Simple file management** - just place files in `data/` folder
- **Automatic upload** to ESP32's SPIFFS filesystem
- **Persistent storage** across reboots

This approach gives you the best of both worlds: significant power savings while maintaining full network connectivity for remote OTA triggers! 