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
  // GPIO CONFIG 
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

// wait for button to be pressed ( this will change to some kind of softwarde signal later)
while (digitalRead(D10)!= HIGH) {
}
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
  // enterBSL();
  // Serial.print(Serial2.read());
  // // sendConnection();
  // delay(100);

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
  // Step 0: Assert BSL_invoke (PA18) high first
  digitalWrite(D12, HIGH);   // PA18 = BSL_invoke, active high
  delay(500);     // hold it for 500ms

  // Step 1: Pulse NRST low to trigger BOOTRST
  digitalWrite(D11, LOW);     // NRST low (reset)
  delay(2);                   // hold reset low; 100µs+ is more than enough (2ms here is safe)

  // Step 2: Release NRST
  digitalWrite(D11, HIGH);    // NRST high — device boots, sees BSL_invoke high

  // Step 3: Keep BSL_invoke high briefly while bootloader starts
  delay(500); // give some time for bootcode to sample and enter BSL
  // Optional: you can pull PA18 low afterward if your application reuses it
}


bool sendConnection() {
  while (Serial2.available()) {
    Serial.println(Serial2.read());
  }
  Serial2.write(0x80); // header

  Serial2.write(0x01); // Length
  Serial2.write(0x00);

  Serial2.write(0x19); // command (all my data)
  uint8_t data = 0x19;
  uint32_t crc32Code = crc32_iso(&data, 1);

Serial2.write((uint8_t)(crc32Code & 0xFF));         // LSB
Serial2.write((uint8_t)((crc32Code >> 8) & 0xFF));
Serial2.write((uint8_t)((crc32Code >> 16) & 0xFF));
Serial2.write((uint8_t)((crc32Code >> 24) & 0xFF)); // MSB


  while (Serial2.available() < 1) {
    delay(1); // wait for response
    digitalWrite(LED_BUILTIN, HIGH);
  }
  digitalWrite(LED_BUILTIN, LOW);
uint32_t Tresponse = 0x00;
  Serial.println("made it past getting a response ");
  while (Serial2.available() > 0) {
  uint8_t response = Serial2.read();
  Tresponse = Tresponse << 8 | response;
  Serial.print("Response code received: 0x");
  Serial.println(response, HEX);
  }
  if (Serial2.available()) {
    Serial.println("is still availabel");
  }
  if (Tresponse == 0x00) {
    Serial.println("SUCCESS: ACK received");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on LED for success
    return true;
  }
  
  // Unknown response
  Serial.print("UNKNOWN response: 0x");
  digitalWrite(LED_BUILTIN, LOW);
  return false;
}