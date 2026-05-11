
#include <SPI.h>
#include <RH_RF95.h>

// Define pins for the RFM96W
#define RFM95_CS 0
#define RFM95_RST 1
#define RFM95_INT 4
#define RF95_FREQ 433.0

// Create an instance of the driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

    // Reset the module
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  // Initialize the radio
  if (!rf95.init()) {
    //Serial.println("RFM96W LoRa radio init failed");
    while (1);
  }
  //Serial.println("RFM96W LoRa radio init OK!");

  // Set frequency
  if (!rf95.setFrequency(RF95_FREQ)) {
    //Serial.println("Failed to set frequency");
    while (1);
  }
  //Serial.print("Frequency set to: ");
  //Serial.println(RF95_FREQ);

  // Set receiver gain
  rf95.setTxPower(23, false); // Not used in receiving but good practice

}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);


  if (Serial.available() > 0) {
    String msg = Serial.readString();
    const char* data = msg.c_str();                 // char*
    uint8_t dataLen = (uint8_t)msg.length();        // uint8_t

    rf95.send((uint8_t*)data, dataLen);
    rf95.waitPacketSent();
  }

  delay(30);

  if (rf95.recv(buf, &len)) {
    Serial.print("Got reply: ");
    Serial.println((char*)buf);
    Serial.print("RSSI: ");
    Serial.println(rf95.lastRssi(), DEC);    
  }

  delay(30);

}
