// Prathik Narsetty
// ESP32 OTA Gateway for MSPM0 Programming - Light Sleep Mode Version
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <stdint.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

// WiFi Configuration
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Web Server for remote trigger and upload
WebServer server(80);

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
bool programmingInProgress = false;
bool firmwareUploaded = false;

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
void enterLightSleep();
void setupGPIO();
void setupSPIFFS();
void setupWiFi();
void setupWebServer();
void handleRoot();
void handleUpload();
void handleTrigger();
void handleStatus();
void handleProgram();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 OTA Gateway - Light Sleep Mode with Web Upload");
  
  // Setup GPIO and SPIFFS
  setupGPIO();
  setupSPIFFS();
  
  // Initialize UART for MSPM0 communication
  Serial2.begin(9600, SERIAL_8N1, D0, D1);
  
  // Setup WiFi and Web Server
  setupWiFi();
  setupWebServer();
  
  Serial.println("Setup complete. Web interface available at:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("Upload firmware through web interface or use external trigger");
  
  // Check if firmware file exists
  if (SPIFFS.exists(FIRMWARE_PATH)) {
    File file = SPIFFS.open(FIRMWARE_PATH, "r");
    Serial.print("Existing firmware file found: ");
    Serial.print(file.size());
    Serial.println(" bytes");
    file.close();
    firmwareUploaded = true;
  } else {
    Serial.println("No firmware file found. Upload one through web interface.");
  }
  
  // Enter light sleep mode
  enterLightSleep();
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Check for external trigger
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(PIN_TRIGGER);
  
  if (lastButtonState == HIGH && currentButtonState == LOW && !programmingInProgress) {
    Serial.println("External trigger detected!");
    triggerProgramming();
  }
  
  lastButtonState = currentButtonState;
  
  // If not programming, enter light sleep periodically
  if (!programmingInProgress) {
    enterLightSleep();
  }
  
  delay(100);
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

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected to WiFi. IP: ");
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/trigger", HTTP_POST, handleTrigger);
  server.on("/program", HTTP_POST, handleProgram);
  server.on("/status", HTTP_GET, handleStatus);
  
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>MSPM0 OTA Gateway</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .section { margin: 30px 0; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }
        .upload-area { border: 2px dashed #007cba; padding: 30px; text-align: center; margin: 20px 0; background: #f8f9fa; border-radius: 5px; }
        .btn { background: #007cba; color: white; padding: 12px 24px; border: none; cursor: pointer; border-radius: 5px; font-size: 16px; }
        .btn:hover { background: #005a87; }
        .btn:disabled { background: #ccc; cursor: not-allowed; }
        .status { margin: 20px 0; padding: 15px; border-radius: 5px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        .progress { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; overflow: hidden; }
        .progress-bar { height: 100%; background: #007cba; width: 0%; transition: width 0.3s; }
        .file-info { margin: 10px 0; padding: 10px; background: #e9ecef; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 MSPM0 OTA Gateway</h1>
        <p>Upload MSPM0 firmware and program devices remotely</p>
        
        <div class="section">
            <h2>📁 Upload Firmware</h2>
            <div class="upload-area">
                <form id="uploadForm" enctype="multipart/form-data">
                    <input type="file" id="firmwareFile" accept=".bin,.hex" required style="margin: 10px;">
                    <br>
                    <button type="submit" class="btn">Upload Firmware</button>
                </form>
                <div class="progress" id="progressBar" style="display: none;">
                    <div class="progress-bar" id="progressBarFill"></div>
                </div>
            </div>
            <div id="fileInfo" class="file-info" style="display: none;"></div>
        </div>
        
        <div class="section">
            <h2>⚡ Program MSPM0</h2>
            <p>Current Status: <span id="status">Idle</span></p>
            <button id="programBtn" class="btn" onclick="programDevice()" disabled>Start Programming</button>
            <div id="result" class="status"></div>
        </div>
        
        <div class="section">
            <h2>📊 System Info</h2>
            <p><strong>ESP32 IP:</strong> <span id="esp32IP">Loading...</span></p>
            <p><strong>Firmware Status:</strong> <span id="firmwareStatus">Checking...</span></p>
            <p><strong>Programming Status:</strong> <span id="programmingStatus">Idle</span></p>
        </div>
    </div>
    
    <script>
        // Update ESP32 IP
        document.getElementById('esp32IP').textContent = window.location.hostname;
        
        // File upload handling
        document.getElementById('uploadForm').onsubmit = function(e) {
            e.preventDefault();
            const fileInput = document.getElementById('firmwareFile');
            const file = fileInput.files[0];
            
            if (!file) {
                showResult('Please select a file', 'error');
                return;
            }
            
            const formData = new FormData();
            formData.append('firmware', file);
            
            // Show progress bar
            document.getElementById('progressBar').style.display = 'block';
            document.getElementById('progressBarFill').style.width = '0%';
            
            // Simulate progress (since we can't get real upload progress easily)
            let progress = 0;
            const progressInterval = setInterval(() => {
                progress += Math.random() * 10;
                if (progress > 90) progress = 90;
                document.getElementById('progressBarFill').style.width = progress + '%';
            }, 100);
            
            fetch('/upload', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                clearInterval(progressInterval);
                document.getElementById('progressBarFill').style.width = '100%';
                
                if (data.success) {
                    showResult('Firmware uploaded successfully! File size: ' + data.size + ' bytes', 'success');
                    document.getElementById('programBtn').disabled = false;
                    updateFirmwareStatus();
                } else {
                    showResult('Upload failed: ' + data.error, 'error');
                }
            })
            .catch(error => {
                clearInterval(progressInterval);
                showResult('Upload error: ' + error, 'error');
            })
            .finally(() => {
                setTimeout(() => {
                    document.getElementById('progressBar').style.display = 'none';
                }, 2000);
            });
        };
        
        // Programming function
        function programDevice() {
            document.getElementById('programmingStatus').textContent = 'Programming...';
            document.getElementById('programBtn').disabled = true;
            showResult('Starting programming...', 'info');
            
            fetch('/program', {
                method: 'POST'
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showResult('Programming completed successfully!', 'success');
                    document.getElementById('programmingStatus').textContent = 'Completed';
                } else {
                    showResult('Programming failed: ' + data.error, 'error');
                    document.getElementById('programmingStatus').textContent = 'Failed';
                }
            })
            .catch(error => {
                showResult('Programming error: ' + error, 'error');
                document.getElementById('programmingStatus').textContent = 'Error';
            })
            .finally(() => {
                document.getElementById('programBtn').disabled = false;
                updateStatus();
            });
        }
        
        function showResult(message, type) {
            const resultDiv = document.getElementById('result');
            resultDiv.textContent = message;
            resultDiv.className = 'status ' + type;
        }
        
        function updateStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('status').textContent = data.status;
                document.getElementById('programmingStatus').textContent = data.programming ? 'Programming' : 'Idle';
            });
        }
        
        function updateFirmwareStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                if (data.firmwareUploaded) {
                    document.getElementById('firmwareStatus').textContent = 'Firmware ready';
                    document.getElementById('programBtn').disabled = false;
                } else {
                    document.getElementById('firmwareStatus').textContent = 'No firmware uploaded';
                    document.getElementById('programBtn').disabled = true;
                }
            });
        }
        
        // Update status every 5 seconds
        setInterval(updateStatus, 5000);
        
        // Initial status update
        updateFirmwareStatus();
    </script>
</body>
</html>
  )";
  
  server.send(200, "text/html", html);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/firmware.bin";
    
    // Open file for writing
    File file = SPIFFS.open(filename, "w");
    if (!file) {
      server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to create file\"}");
      return;
    }
    file.close();
    
    Serial.println("Starting firmware upload...");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = SPIFFS.open("/firmware.bin", "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    File file = SPIFFS.open("/firmware.bin", "r");
    if (file) {
      size_t fileSize = file.size();
      file.close();
      
      firmwareUploaded = true;
      String response = "{\"success\":true,\"size\":" + String(fileSize) + "}";
      server.send(200, "application/json", response);
      
      Serial.print("Firmware uploaded successfully: ");
      Serial.print(fileSize);
      Serial.println(" bytes");
    } else {
      server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to read uploaded file\"}");
    }
  }
}

void handleTrigger() {
  if (programmingInProgress) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Programming already in progress\"}");
    return;
  }
  
  triggerProgramming();
  server.send(200, "application/json", "{\"success\":true}");
}

void handleProgram() {
  if (programmingInProgress) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Programming already in progress\"}");
    return;
  }
  
  if (!firmwareUploaded || !SPIFFS.exists(FIRMWARE_PATH)) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No firmware uploaded\"}");
    return;
  }
  
  triggerProgramming();
  server.send(200, "application/json", "{\"success\":true}");
}

void handleStatus() {
  String status = programmingInProgress ? "Programming" : "Idle";
  String response = "{\"status\":\"" + status + "\",\"firmwareUploaded\":" + String(firmwareUploaded ? "true" : "false") + ",\"programming\":" + String(programmingInProgress ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

void enterLightSleep() {
  Serial.println("Entering light sleep mode...");
  Serial.println("WiFi remains active for remote triggers");
  
  // Turn off LED to indicate sleep
  digitalWrite(PIN_LED, LOW);
  
  // Configure light sleep with WiFi wake
  esp_sleep_enable_timer_wakeup(10000000); // 10 seconds
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TRIGGER, 0); // Wake on LOW
  
  // Enter light sleep (WiFi stays active)
  esp_light_sleep_start();
  
  // Wake up and continue
  Serial.println("Waking from light sleep...");
}

void triggerProgramming() {
  if (programmingInProgress) {
    Serial.println("Programming already in progress!");
    return;
  }
  
  programmingInProgress = true;
  Serial.println("=== TRIGGERING OTA PROGRAMMING ===");
  
  // Turn on LED to indicate activity
  digitalWrite(PIN_LED, HIGH);
  delay(1000);
  
  // Check if firmware file exists
  if (!SPIFFS.exists(FIRMWARE_PATH)) {
    Serial.println("ERROR: No firmware.bin file found!");
    Serial.println("Please upload firmware through web interface");
    digitalWrite(PIN_LED, LOW);
    programmingInProgress = false;
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
  
  programmingInProgress = false;
  
  // Wait a bit then return to normal operation
  delay(5000);
  digitalWrite(PIN_LED, LOW);
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