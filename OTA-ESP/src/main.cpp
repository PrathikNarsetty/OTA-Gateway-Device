// Prathik Narsetty
// ESP32 OTA Gateway for MSPM0 Programming - Sleep Mode Version
#include <Arduino.h>
#include <SPIFFS.h>
#include <stdint.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// GPIO Configuration
#define PIN_PA18 D12      // BSL invoke pin
#define PIN_NRST D11      // Reset pin
#define PIN_TRIGGER D10   // OTA trigger pin (external signal)
#define PIN_LED LED_BUILTIN

// BSL Protocol Constants
#define PACKET_HEADER (0x80)
#define CMD_CONNECTION (0x12)
#define CMD_GET_ID (0x19)
#define CMD_RX_PASSWORD (0x21)
#define CMD_MASS_ERASE (0x15)
#define CMD_PROGRAMDATA (0x20)
#define CMD_START_APP (0x40)

// BSL Password (all 0xFF for unlocked device)
const uint8_t BSL_PW_RESET[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Global Variables
uint8_t BSL_TX_buffer[256];
uint8_t BSL_RX_buffer[256];
const char* FIRMWARE_PATH = "/firmware.bin";

// BSL Error Codes
enum {
    eBSL_success = 0,
    eBSL_unknownError = 7
};
typedef uint8_t BSL_error_t;

// Function declarations
uint32_t crc32_iso(const uint8_t *data, size_t len);
void enterBSL();
bool performBSLProgramming();
BSL_error_t bslConnection();
BSL_error_t bslGetID();
BSL_error_t bslLoadPassword();
BSL_error_t bslMassErase();
BSL_error_t bslProgramData();
BSL_error_t bslStartApp();
BSL_error_t bslGetResponse();
void enterSleepMode();
void setupGPIO();
void setupSPIFFS();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 OTA Gateway - Sleep Mode");
  
  // Setup GPIO and SPIFFS
  setupGPIO();
  setupSPIFFS();
  
  // Initialize UART for MSPM0 communication
  Serial2.begin(9600, SERIAL_8N1, D0, D1);
  
  Serial.println("Setup complete. Waiting for OTA trigger...");
  Serial.println("Place firmware.bin in data/ folder and upload with SPIFFS");
  
  // Check if firmware file exists
  if (SPIFFS.exists(FIRMWARE_PATH)) {
    File file = SPIFFS.open(FIRMWARE_PATH, "r");
    Serial.print("Firmware file found: ");
    Serial.print(file.size());
    Serial.println(" bytes");
    file.close();
  } else {
    Serial.println("WARNING: No firmware.bin file found!");
    Serial.println("Please place firmware.bin in the data/ folder");
  }
  
  // Enter sleep mode immediately
  enterSleepMode();
}

void loop() {
  // This should never be reached in normal operation
  // The device will wake up, perform OTA, then go back to sleep
  Serial.println("Unexpected wake - going back to sleep");
  enterSleepMode();
}

void setupGPIO() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_PA18, OUTPUT);
  pinMode(PIN_NRST, OUTPUT);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  
  digitalWrite(PIN_NRST, HIGH); // NRST high initially
  digitalWrite(PIN_LED, LOW);   // LED off initially
}

void setupSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
}

void enterSleepMode() {
  Serial.println("Entering deep sleep mode...");
  Serial.println("Connect PIN_TRIGGER (D10) to GND to wake and program");
  
  // Configure wake-up source
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TRIGGER, 0); // Wake on LOW
  
  // Turn off LED to indicate sleep
  digitalWrite(PIN_LED, LOW);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

void wakeUpRoutine() {
  Serial.println("=== WAKING UP - OTA TRIGGERED ===");
  
  // Turn on LED to indicate activity
  digitalWrite(PIN_LED, HIGH);
  delay(1000);
  
  // Check if firmware file exists
  if (!SPIFFS.exists(FIRMWARE_PATH)) {
    Serial.println("ERROR: No firmware.bin file found!");
    Serial.println("Please place firmware.bin in the data/ folder");
    digitalWrite(PIN_LED, LOW);
    enterSleepMode();
    return;
  }
  
  // Perform BSL programming
  if (performBSLProgramming()) {
    Serial.println("OTA Programming completed successfully!");
    digitalWrite(PIN_LED, HIGH); // Keep LED on to indicate success
  } else {
    Serial.println("OTA Programming failed!");
    digitalWrite(PIN_LED, LOW);
  }
  
  // Wait a bit then go back to sleep
  delay(5000);
  enterSleepMode();
}

uint32_t crc32_iso(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return ~crc;
}

void enterBSL() {
  Serial.println("Entering BSL mode...");
  
  // Step 0: Assert BSL_invoke (PA18) high first
  digitalWrite(PIN_PA18, HIGH);   // PA18 = BSL_invoke, active high
  delay(500);     // hold it for 500ms

  // Step 1: Pulse NRST low to trigger BOOTRST
  digitalWrite(PIN_NRST, LOW);     // NRST low (reset)
  delay(2);                   // hold reset low; 100µs+ is more than enough (2ms here is safe)

  // Step 2: Release NRST
  digitalWrite(PIN_NRST, HIGH);    // NRST high — device boots, sees BSL_invoke high

  // Step 3: Keep BSL_invoke high briefly while bootloader starts
  delay(500); // give some time for bootcode to sample and enter BSL
}

bool performBSLProgramming() {
  Serial.println("=== Starting BSL Programming ===");
  
  // Step 1: Enter BSL mode
  enterBSL();
  delay(1000);
  
  // Step 2: Establish BSL connection
  if (bslConnection() != eBSL_success) {
    Serial.println("BSL connection failed");
    return false;
  }
  
  // Step 3: Get device ID
  if (bslGetID() != eBSL_success) {
    Serial.println("Failed to get device ID");
    return false;
  }
  
  // Step 4: Load password
  if (bslLoadPassword() != eBSL_success) {
    Serial.println("Failed to load password");
    return false;
  }
  
  // Step 5: Mass erase
  if (bslMassErase() != eBSL_success) {
    Serial.println("Mass erase failed");
    return false;
  }
  
  // Step 6: Program firmware
  if (bslProgramData() != eBSL_success) {
    Serial.println("Firmware programming failed");
    return false;
  }
  
  // Step 7: Start application
  if (bslStartApp() != eBSL_success) {
    Serial.println("Failed to start application");
    return false;
  }
  
  Serial.println("=== BSL Programming Completed Successfully ===");
  return true;
}

BSL_error_t bslConnection() {
  Serial.println("Sending BSL connection packet...");
  
  BSL_TX_buffer[0] = PACKET_HEADER;
  BSL_TX_buffer[1] = 0x01; // Length LSB
  BSL_TX_buffer[2] = 0x00; // Length MSB
  BSL_TX_buffer[3] = CMD_CONNECTION;
  
  uint32_t crc = crc32_iso(&BSL_TX_buffer[3], 1);
  BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
  BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
  BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
  BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
  
  Serial2.write(BSL_TX_buffer, 8);
  
  return bslGetResponse();
}

BSL_error_t bslGetID() {
  Serial.println("Sending Get ID packet...");
  
  BSL_TX_buffer[0] = PACKET_HEADER;
  BSL_TX_buffer[1] = 0x01; // Length LSB
  BSL_TX_buffer[2] = 0x00; // Length MSB
  BSL_TX_buffer[3] = CMD_GET_ID;
  
  uint32_t crc = crc32_iso(&BSL_TX_buffer[3], 1);
  BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
  BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
  BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
  BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
  
  Serial2.write(BSL_TX_buffer, 8);
  
  // Read response (device ID)
  delay(100);
  int bytesRead = 0;
  while (Serial2.available() && bytesRead < 32) {
    BSL_RX_buffer[bytesRead] = Serial2.read();
    bytesRead++;
  }
  
  Serial.print("Device ID received: ");
  Serial.print(bytesRead);
  Serial.println(" bytes");
  
  return eBSL_success;
}

BSL_error_t bslLoadPassword() {
  Serial.println("Sending password packet...");
  
  BSL_TX_buffer[0] = PACKET_HEADER;
  BSL_TX_buffer[1] = 0x21; // Length LSB (32 + 1)
  BSL_TX_buffer[2] = 0x00; // Length MSB
  BSL_TX_buffer[3] = CMD_RX_PASSWORD;
  
  // Copy password
  memcpy(&BSL_TX_buffer[4], BSL_PW_RESET, 32);
  
  uint32_t crc = crc32_iso(&BSL_TX_buffer[3], 33);
  BSL_TX_buffer[36] = (crc >> 0) & 0xFF;
  BSL_TX_buffer[37] = (crc >> 8) & 0xFF;
  BSL_TX_buffer[38] = (crc >> 16) & 0xFF;
  BSL_TX_buffer[39] = (crc >> 24) & 0xFF;
  
  Serial2.write(BSL_TX_buffer, 40);
  
  return bslGetResponse();
}

BSL_error_t bslMassErase() {
  Serial.println("Sending mass erase packet...");
  
  BSL_TX_buffer[0] = PACKET_HEADER;
  BSL_TX_buffer[1] = 0x01; // Length LSB
  BSL_TX_buffer[2] = 0x00; // Length MSB
  BSL_TX_buffer[3] = CMD_MASS_ERASE;
  
  uint32_t crc = crc32_iso(&BSL_TX_buffer[3], 1);
  BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
  BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
  BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
  BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
  
  Serial2.write(BSL_TX_buffer, 8);
  
  return bslGetResponse();
}

BSL_error_t bslProgramData() {
  Serial.println("Programming firmware data...");
  
  File file = SPIFFS.open(FIRMWARE_PATH, "r");
  if (!file) {
    Serial.println("Failed to open firmware file");
    return eBSL_unknownError;
  }
  
  const int blockSize = 128; // BSL block size
  uint8_t buffer[blockSize];
  uint32_t address = 0x00000000; // Starting address
  
  Serial.print("Programming ");
  Serial.print(file.size());
  Serial.println(" bytes");
  
  while (file.available()) {
    int bytesRead = file.read(buffer, blockSize);
    
    // Prepare data block packet
    BSL_TX_buffer[0] = PACKET_HEADER;
    BSL_TX_buffer[1] = (bytesRead + 5) & 0xFF; // Length LSB (data + address + length)
    BSL_TX_buffer[2] = ((bytesRead + 5) >> 8) & 0xFF; // Length MSB
    BSL_TX_buffer[3] = CMD_PROGRAMDATA;
    
    // Address (4 bytes, little-endian)
    BSL_TX_buffer[4] = address & 0xFF;
    BSL_TX_buffer[5] = (address >> 8) & 0xFF;
    BSL_TX_buffer[6] = (address >> 16) & 0xFF;
    BSL_TX_buffer[7] = (address >> 24) & 0xFF;
    
    // Data length (2 bytes, little-endian)
    BSL_TX_buffer[8] = bytesRead & 0xFF;
    BSL_TX_buffer[9] = (bytesRead >> 8) & 0xFF;
    
    // Copy data
    memcpy(&BSL_TX_buffer[10], buffer, bytesRead);
    
    // Calculate CRC
    uint32_t crc = crc32_iso(&BSL_TX_buffer[3], bytesRead + 7);
    BSL_TX_buffer[10 + bytesRead] = (crc >> 0) & 0xFF;
    BSL_TX_buffer[10 + bytesRead + 1] = (crc >> 8) & 0xFF;
    BSL_TX_buffer[10 + bytesRead + 2] = (crc >> 16) & 0xFF;
    BSL_TX_buffer[10 + bytesRead + 3] = (crc >> 24) & 0xFF;
    
    // Send packet
    Serial2.write(BSL_TX_buffer, 10 + bytesRead + 4);
    
    // Wait for response
    if (bslGetResponse() != eBSL_success) {
      Serial.println("Data block programming failed");
      file.close();
      return eBSL_unknownError;
    }
    
    address += bytesRead;
    Serial.print("Programmed ");
    Serial.print(address);
    Serial.println(" bytes");
  }
  
  file.close();
  return eBSL_success;
}

BSL_error_t bslStartApp() {
  Serial.println("Sending start app packet...");
  
  BSL_TX_buffer[0] = PACKET_HEADER;
  BSL_TX_buffer[1] = 0x01; // Length LSB
  BSL_TX_buffer[2] = 0x00; // Length MSB
  BSL_TX_buffer[3] = CMD_START_APP;
  
  uint32_t crc = crc32_iso(&BSL_TX_buffer[3], 1);
  BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
  BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
  BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
  BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
  
  Serial2.write(BSL_TX_buffer, 8);
  
  return eBSL_success;
}

BSL_error_t bslGetResponse() {
  delay(100);
  
  int bytesRead = 0;
  while (Serial2.available() && bytesRead < 16) {
    BSL_RX_buffer[bytesRead] = Serial2.read();
    bytesRead++;
  }
  
  if (bytesRead >= 5) {
    uint8_t ack = BSL_RX_buffer[4];
    Serial.print("Response ACK: 0x");
    Serial.println(ack, HEX);
    return ack;
  } else {
    Serial.println("No response received");
    return eBSL_unknownError;
  }
}

void setup() {
  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // Woken up by external trigger
    Serial.begin(115200);
    delay(1000);
    wakeUpRoutine();
  } else {
    // First boot or other wake reason
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 OTA Gateway - Sleep Mode");
    
    // Setup GPIO and SPIFFS
    setupGPIO();
    setupSPIFFS();
    
    // Initialize UART for MSPM0 communication
    Serial2.begin(9600, SERIAL_8N1, D0, D1);
    
    Serial.println("Setup complete. Waiting for OTA trigger...");
    Serial.println("Place firmware.bin in data/ folder and upload with SPIFFS");
    
    // Check if firmware file exists
    if (SPIFFS.exists(FIRMWARE_PATH)) {
      File file = SPIFFS.open(FIRMWARE_PATH, "r");
      Serial.print("Firmware file found: ");
      Serial.print(file.size());
      Serial.println(" bytes");
      file.close();
    } else {
      Serial.println("WARNING: No firmware.bin file found!");
      Serial.println("Please place firmware.bin in the data/ folder");
    }
    
    // Enter sleep mode immediately
    enterSleepMode();
  }
}