// Prathik Narsetty
#include <Arduino.h>
#include <stdint.h>

// function declarations
uint32_t crc32_iso(const uint8_t *data, size_t len);
void enterBSL();
bool sendConnection();

void setup() {
  Serial.begin(115200);
  delay(1000); // wait for serial to be ready
  Serial.println("Hello World");

  // so far I have setup linkage to my serial console. Let me setup GPIO pins for driving PA18 and NRST
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D12, OUTPUT); // I will use D12 to control PA18
  pinMode(D11, OUTPUT); // let me use D11 ro control NRST
  pinMode(D10, INPUT_PULLUP);
  digitalWrite(D11, HIGH); // make sure NRST is high  

  
// setup UART linkage
// Here is the information I have gathered from the BSL datasheet
/*
MSPM0 flash-based UART is enabled with following configuration by default:
• Baud rate: 9600bps
• Data width: 8 bit
• One stop bit
• No parity
*/
digitalWrite(LED_BUILTIN, HIGH);
while (digitalRead(D10)!= HIGH) {

}
digitalWrite(LED_BUILTIN, LOW);
  Serial2.begin(9600, SERIAL_8N1, D0, D1); // TX and RX on ESP32
  // make sure to aslo connect grounds 
  enterBSL();
  if (sendConnection() == true) {
    Serial.println("Connection successful - LED should be ON");
  } else {
    Serial.println("Connection failed - LED should be blinking or OFF");
  }
}

void loop() {

}



uint32_t crc32_iso(const uint8_t *data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  while (len--)
  {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return ~crc;
}

void enterBSL() {
  digitalWrite(D12, LOW);    // Step 0: ensure PA18 LOW
  digitalWrite(D11, LOW);    // Step 1: NRST LOW (reset)
  delay(10);                 // Hold reset for ~10 ms

  digitalWrite(D12, HIGH);   // Step 2: PA18 HIGH (before NRST is released)
  delay(1);                  // Small delay to ensure it's latched

  digitalWrite(D11, HIGH);   // Step 3: Release NRST
  delay(10);                 // Step 4: Hold PA18 high for a bit

  // Optional: PA18 can be pulled low now or just left high
}

bool sendConnection() {
  Serial2.write(0x80); // header

  Serial2.write(0x01); // Length
  Serial2.write(0x00);

  Serial2.write(0x12); // command (all my data)
  uint8_t data = 0x12;
  uint32_t crc32Code = crc32_iso(&data, 1);

Serial2.write((uint8_t)(crc32Code & 0xFF));         // LSB
Serial2.write((uint8_t)((crc32Code >> 8) & 0xFF));
Serial2.write((uint8_t)((crc32Code >> 16) & 0xFF));
Serial2.write((uint8_t)((crc32Code >> 24) & 0xFF)); // MSB

  while (Serial2.available() < 1) {
    delay(1);
  }

  Serial.println("made it past getting a response ");
  while (Serial2.available() > 1) {
  uint8_t response = Serial2.read();
  Serial.print("Response code received: 0x");
  Serial.println(response, HEX);
  }
  // Display the actual response code
  uint8_t response;
  Serial.print("Response code received: 0x");
  Serial.println(response, HEX);
  
  if (response == 0x00) {
    Serial.println("SUCCESS: ACK received");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on LED for success
    return true;
  }
  
  // Handle error codes (0x5x range)
  if (response >= 0x50 && response <= 0x5F) {
    Serial.print("ERROR: BSL error code 0x");
    Serial.println(response, HEX);
    
    // Blink LED to indicate error
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    
    return false;
  }
  
  // Unknown response
  Serial.print("UNKNOWN response: 0x");
  Serial.println(response, HEX);
  digitalWrite(LED_BUILTIN, LOW);
  return false;
}