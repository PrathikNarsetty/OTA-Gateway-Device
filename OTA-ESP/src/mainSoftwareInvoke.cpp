// // Prathik Narsetty
// // ESP32 BSL Software Invoke Implementation
// // Based on TI MSPM0 Software Invoke Reference

// #include <Arduino.h>
// #include <stdint.h>

// // BSL Protocol Constants
// #define PACKET_HEADER (0x80)
// #define CMD_CONNECTION (0x12)
// #define CMD_GET_ID (0x19)
// #define CMD_RX_PASSWORD (0x21)
// #define CMD_MASS_ERASE (0x15)
// #define CMD_PROGRAMDATA (0x20)
// #define CMD_START_APP (0x40)

// // Packet Structure
// #define CMD_BYTE (1)
// #define HDR_LEN_CMD_BYTES (4)
// #define CRC_BYTES (4)
// #define PASSWORD_SIZE (32)
// #define ACK_BYTE (1)
// #define ID_BACK (24)
// #define ADDRS_BYTES (4)

// // BSL Error Codes
// enum {
//     eBSL_success = 0,
//     eBSL_unknownError = 7
// };
// typedef uint8_t BSL_error_t;

// // Global Variables
// uint8_t BSL_TX_buffer[256];
// uint8_t BSL_RX_buffer[256];
// uint16_t BSL_MAX_BUFFER_SIZE = 0;

// // BSL Password (all 0xFF for unlocked device)
// const uint8_t BSL_PW_RESET[32] = {
//     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
// };

// // Function Declarations
// uint32_t softwareCRC(const uint8_t *data, uint8_t length);
// void softwareTrigger();
// BSL_error_t bslConnection();
// BSL_error_t bslGetID();
// BSL_error_t bslLoadPassword();
// BSL_error_t bslMassErase();
// BSL_error_t bslStartApp();
// BSL_error_t bslGetResponse();
// void performBSLProgramming();

// // CRC32 Implementation (same as TI reference)
// #define CRC32_POLY 0xEDB88320
// uint32_t softwareCRC(const uint8_t *data, uint8_t length) {
//     uint32_t ii, jj, byte, crc, mask;
    
//     crc = 0xFFFFFFFF;
    
//     for (ii = 0; ii < length; ii++) {
//         byte = data[ii];
//         crc = crc ^ byte;
        
//         for (jj = 0; jj < 8; jj++) {
//             mask = -(crc & 1);
//             crc = (crc >> 1) ^ (CRC32_POLY & mask);
//         }
//     }
    
//     return crc;
// }

// // Software Trigger - sends 0x22 to invoke BSL mode
// void softwareTrigger() {
//     Serial.println("Sending software trigger (0x22)...");
//     Serial2.write(0x22);
//     Serial.println("Software trigger sent");
    
//     // MSPM0 will wait ~1 second, then reset and enter BSL mode
//     Serial.println("Waiting for MSPM0 to reset and enter BSL mode...");
//     delay(2000); // Wait for reset and BSL entry
// }

// // BSL Connection
// BSL_error_t bslConnection() {
//     Serial.println("Sending BSL connection packet...");
    
//     BSL_TX_buffer[0] = PACKET_HEADER;
//     BSL_TX_buffer[1] = 0x01; // Length LSB
//     BSL_TX_buffer[2] = 0x00; // Length MSB
//     BSL_TX_buffer[3] = CMD_CONNECTION;
    
//     uint32_t crc = softwareCRC(&BSL_TX_buffer[3], 1);
//     BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
//     BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
//     BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
//     BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
    
//     Serial2.write(BSL_TX_buffer, 8);
//     Serial.println("Connection packet sent");
    
//     return eBSL_success;
// }

// // Get Device ID
// BSL_error_t bslGetID() {
//     Serial.println("Sending Get ID packet...");
    
//     BSL_TX_buffer[0] = PACKET_HEADER;
//     BSL_TX_buffer[1] = 0x01; // Length LSB
//     BSL_TX_buffer[2] = 0x00; // Length MSB
//     BSL_TX_buffer[3] = CMD_GET_ID;
    
//     uint32_t crc = softwareCRC(&BSL_TX_buffer[3], 1);
//     BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
//     BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
//     BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
//     BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
    
//     Serial2.write(BSL_TX_buffer, 8);
    
//     // Read response
//     delay(100);
//     int bytesRead = 0;
//     while (Serial2.available() && bytesRead < 32) {
//         BSL_RX_buffer[bytesRead] = Serial2.read();
//         bytesRead++;
//     }
    
//     Serial.print("Get ID response: ");
//     Serial.print(bytesRead);
//     Serial.println(" bytes");
    
//     return eBSL_success;
// }

// // Load Password
// BSL_error_t bslLoadPassword() {
//     Serial.println("Sending password packet...");
    
//     BSL_TX_buffer[0] = PACKET_HEADER;
//     BSL_TX_buffer[1] = 0x21; // Length LSB (32 + 1)
//     BSL_TX_buffer[2] = 0x00; // Length MSB
//     BSL_TX_buffer[3] = CMD_RX_PASSWORD;
    
//     // Copy password
//     memcpy(&BSL_TX_buffer[4], BSL_PW_RESET, 32);
    
//     uint32_t crc = softwareCRC(&BSL_TX_buffer[3], 33);
//     BSL_TX_buffer[36] = (crc >> 0) & 0xFF;
//     BSL_TX_buffer[37] = (crc >> 8) & 0xFF;
//     BSL_TX_buffer[38] = (crc >> 16) & 0xFF;
//     BSL_TX_buffer[39] = (crc >> 24) & 0xFF;
    
//     Serial2.write(BSL_TX_buffer, 40);
    
//     return bslGetResponse();
// }

// // Mass Erase
// BSL_error_t bslMassErase() {
//     Serial.println("Sending mass erase packet...");
    
//     BSL_TX_buffer[0] = PACKET_HEADER;
//     BSL_TX_buffer[1] = 0x01; // Length LSB
//     BSL_TX_buffer[2] = 0x00; // Length MSB
//     BSL_TX_buffer[3] = CMD_MASS_ERASE;
    
//     uint32_t crc = softwareCRC(&BSL_TX_buffer[3], 1);
//     BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
//     BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
//     BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
//     BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
    
//     Serial2.write(BSL_TX_buffer, 8);
    
//     return bslGetResponse();
// }

// // Start Application
// BSL_error_t bslStartApp() {
//     Serial.println("Sending start app packet...");
    
//     BSL_TX_buffer[0] = PACKET_HEADER;
//     BSL_TX_buffer[1] = 0x01; // Length LSB
//     BSL_TX_buffer[2] = 0x00; // Length MSB
//     BSL_TX_buffer[3] = CMD_START_APP;
    
//     uint32_t crc = softwareCRC(&BSL_TX_buffer[3], 1);
//     BSL_TX_buffer[4] = (crc >> 0) & 0xFF;
//     BSL_TX_buffer[5] = (crc >> 8) & 0xFF;
//     BSL_TX_buffer[6] = (crc >> 16) & 0xFF;
//     BSL_TX_buffer[7] = (crc >> 24) & 0xFF;
    
//     Serial2.write(BSL_TX_buffer, 8);
    
//     return eBSL_success;
// }

// // Get Response
// BSL_error_t bslGetResponse() {
//     delay(100);
    
//     int bytesRead = 0;
//     while (Serial2.available() && bytesRead < 16) {
//         BSL_RX_buffer[bytesRead] = Serial2.read();
//         bytesRead++;
//     }
    
//     if (bytesRead >= 9) {
//         uint8_t ack = BSL_RX_buffer[4];
//         Serial.print("Response ACK: 0x");
//         Serial.println(ack, HEX);
//         return ack;
//     } else {
//         Serial.println("No response received");
//         return eBSL_unknownError;
//     }
// }

// // Main BSL Programming Sequence
// void performBSLProgramming() {
//     Serial.println("=== BSL Programming Sequence ===");
    
//     // Step 1: Software trigger
//     softwareTrigger();
    
//     // Step 2: BSL Connection
//     bslConnection();
//     delay(100);
    
//     // Step 3: Check if we get a response (indicates BSL mode)
//     if (Serial2.available()) {
//         uint8_t response = Serial2.read();
//         Serial.print("BSL response: 0x");
//         Serial.println(response, HEX);
        
//         if (response == 0x00) { // Success
//             Serial.println("BSL connection successful!");
            
//             // Step 4: Get Device ID
//             bslGetID();
//             delay(100);
            
//             // Step 5: Load Password
//             if (bslLoadPassword() == 0x00) {
//                 Serial.println("Password loaded successfully");
                
//                 // Step 6: Mass Erase
//                 if (bslMassErase() == 0x00) {
//                     Serial.println("Mass erase completed");
                    
//                     // Step 7: Start Application
//                     bslStartApp();
//                     Serial.println("Application started");
//                     digitalWrite(LED_BUILTIN, HIGH); // Success LED
//                 } else {
//                     Serial.println("Mass erase failed");
//                     digitalWrite(LED_BUILTIN, LOW);
//                 }
//             } else {
//                 Serial.println("Password load failed");
//                 digitalWrite(LED_BUILTIN, LOW);
//             }
//         } else {
//             Serial.println("BSL connection failed");
//             digitalWrite(LED_BUILTIN, LOW);
//         }
//     } else {
//         Serial.println("No response from MSPM0 - not in BSL mode");
//         digitalWrite(LED_BUILTIN, LOW);
//     }
    
//     Serial.println("=== Programming Sequence Complete ===");
// }

// // Arduino Setup
// void setup() {
//     Serial.begin(115200);
//     delay(1000);
//     Serial.println("ESP32 BSL Software Invoke");
//     Serial.println("==========================");
    
//     // Initialize UART2 for MSPM0 communication
//     Serial2.begin(9600, SERIAL_8N1, D0, D1);
//     Serial.println("UART2 initialized at 9600 baud");
    
//     // GPIO Configuration
//     pinMode(LED_BUILTIN, OUTPUT);
//     pinMode(D10, INPUT_PULLUP);
    
//     digitalWrite(LED_BUILTIN, LOW);
    
//     Serial.println("Connect ESP32 D0->MSPM0 PA11, ESP32 D1->MSPM0 PA10");
//     Serial.println("Press button on D10 to start BSL programming");
//     Serial.println();
// }

// // Arduino Loop
// void loop() {
//     // Check for button press
//     static bool lastButtonState = HIGH;
//     bool currentButtonState = digitalRead(D10);
    
//     // Detect button press (transition from HIGH to LOW)
//     if (lastButtonState == HIGH && currentButtonState == LOW) {
//         Serial.println("Button pressed - starting BSL programming...");
//         delay(500); // Debounce
//         performBSLProgramming();
//     }
    
//     lastButtonState = currentButtonState;
//     delay(100);
// } 