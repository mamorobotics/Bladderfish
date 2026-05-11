#include <Wire.h>
#include "Adafruit_MPRLS.h"
#include <RTCZero.h>

#include <SPI.h>
#include <RH_RF95.h>

#include <iostream>
#include <string>
#include <cstring>

#define ENABLE_MOTOR 2
#define MOTORA1 3
#define MOTORA2 5

#define RFM95_CS 0
#define RFM95_RST 1
#define RFM95_INT 4
#define RF95_FREQ 433.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

RTCZero rtc;


#define RESET_PIN  -1 
#define EOC_PIN    -1 
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

char TEAM_ID[7] = "TN9999";

int state = 0; 

int packetTime[2][10];
int packetDepth[10];
int packetCounter = 0;


int pumpPin = 5;

// 0: waiting for command to do a vertical profile
// 1: doing a vertical profile
// 2: sending data packets after vertical profile


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial) { }  // if using native USB boards
  Serial.println("BOOT"); 

  // Radio:

 
  setupRadio();
  Serial.println("Radio setup!");

  // Motor:

  pinMode(ENABLE_MOTOR, OUTPUT);
  pinMode(MOTORA1, OUTPUT);
  pinMode(MOTORA2, OUTPUT);

  digitalWrite(ENABLE_MOTOR, LOW);

  Serial.println("Motor Setup");


  // Clock:

  rtc.begin();

  rtc.setHours(0);
  rtc.setMinutes(0);
  rtc.setSeconds(0);

  Serial.println("Clock Setup");

  // Pressure sensor: 

  if (! mpr.begin()) {
    Serial.println("Failed to communicate with MPRLS sensor, check wiring?");
    while (1) {
      delay(10);
    }
  }
  Serial.println("Found MPRLS sensor");


  

}


void setupRadio() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  delay(100);

  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);


  while (!rf95.init()) {
    Serial.println("LoRa radio init failed"); 
    while (1);
  }
  Serial.println("LoRa radio init OK!");


  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void loop() {
  // put your main code here, to run repeatedly:
  

  if (state == 0) {
    waitCommand();
    delay(500);
  }
  if (state == 1) {
    verticalProfile();
    state++;
    Serial.println("Profile Done");
  }
  if (state == 2) {
    Serial.println("Sending data");
    sendData();
    state = 0;
  }
}


void waitCommand() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (rf95.waitAvailableTimeout(1000)) {
    if (rf95.recv(buf, &len))
    {
      Serial.print("Recieved: ");
      Serial.println((char*)buf);

      if (strcmp((char*)buf, "start") == 0) {
        state = 1; // begin vertical profile
      }


      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
  }
}

float readPressure(){
  float pressure_hPa = mpr.readPressure();
  return pressure_hPa;
}


void savePacket() {
  packetTime[packetCounter][0] = rtc.getMinutes();
  packetTime[packetCounter][1] = rtc.getSeconds();
  packetDepth[packetCounter] = (int) readPressure();
  packetCounter++;
}

void verticalProfile() {
  // fill the bladder
  Serial.println("Vertical Profile Start");

  pump(5000, true);
  savePacket();
  pump(5000, true);
  savePacket();

  

  delay(5000);
  savePacket();

  // neutral

  
  Serial.println("Neutral");
  pump(5000, false);
  savePacket();
  

  // wait 10 seconds

  delay(5000);
  savePacket();
  delay(5000);
  savePacket();

  // empty the bladder
  Serial.println("Float");
  pump(5000, false);
  savePacket();

}


void pump(int ms, bool in) {
  // turn on pump for in or out

  digitalWrite(ENABLE_MOTOR, HIGH);

  if (in) {
    digitalWrite(MOTORA1, LOW);
    digitalWrite(MOTORA2, HIGH);
  }

  else {
    digitalWrite(MOTORA1, HIGH);
    digitalWrite(MOTORA2, LOW);
  }

  delay(ms);

  // turn off pump
  digitalWrite(ENABLE_MOTOR, LOW);
}

void sendData() {
  for (int i = 0; i < packetCounter; i++) {
    
    char msg[32] = {0};
    char packetStuff[3][8];


    strcat(msg, TEAM_ID);
    strcat(msg, "_");
    itoa(packetTime[i][0], packetStuff[0], 10);
    itoa(packetTime[i][1], packetStuff[1], 10);
    itoa(packetDepth[i], packetStuff[2], 10);
    strcat(msg, packetStuff[0]);
    strcat(msg, ":");
    strcat(msg, packetStuff[1]);
    strcat(msg, "_");
    strcat(msg, packetStuff[2]);

    Serial.println(msg);

    rf95.send((uint8_t *)msg, strlen(msg));
    rf95.waitPacketSent();
    Serial.println("Sent packet")
    delay(300);
  }

  packetCounter = 0;
}
