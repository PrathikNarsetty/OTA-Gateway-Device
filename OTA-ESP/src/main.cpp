// Prathik Narsetty
#include <Arduino.h>


// function declarations
void resetESP();


void setup() {
  Serial.begin(115200);
  delay(1000); // wait for serial to be ready
  Serial.println("Hello World");
  // so far I have setup linkage to my serial console. Let me setup GPIO pins for driving PA18 and NRST
  pinMode(D12, OUTPUT); // I will use D12 to control PA18
  pinMode(D11, OUTPUT); // let me use D13 ro control NRST
  
// setup UART linkage
// Here is the information I have gathered from the BSL datasheet
/*
MSPM0 flash-based UART is enabled with following configuration by default:
• Baud rate: 9600bps
• Data width: 8 bit
• One stop bit
• No parity
*/
  Serial2.begin(9600, SERIAL_8N1, D0, D1); // TX and RX on ESP32
  // make sure to aslo connect grounds 
  delay(1000);
  enterBSL();
}

void loop() {

}

// function definitions
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

