#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <Wire.h>
#include "Adafruit_MPRLS.h"
#include <RTCZero.h>

// ================== WIFI ==================
char ssid[] = "midnightRobotics";
char pass[] = "hughhughhughhugh";

WiFiServer server(80);
int status = WL_IDLE_STATUS;

// ================== HARDWARE ==================
#define ENABLE_MOTOR 2
#define MOTORA1 3
#define MOTORA2 5

RTCZero rtc;

#define RESET_PIN  -1 
#define EOC_PIN    -1 
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

// ================== STATE ==================
int state = 0;
// 0 = idle
// 1 = vertical profile
// 2 = send data
// 3 = fill neutral
// 4 = test reply
// 5 = pump in 5 seconds
// 6 = pump out 5 seconds

// ================== LOGGING ==================
char TEAM_ID[7] = "TN9999";

int packetTime[10][2];
int packetDepth[10];
int packetCounter = 0;

// ================== NON-BLOCKING PUMP ==================
bool pumpRunning = false;
bool pumpDirectionIn = true;
unsigned long pumpStartTime = 0;
unsigned long pumpDuration = 0;

// ================== PROFILE CONTROL ==================
int profileStep = 0;
unsigned long stepStartTime = 0;

// =====================================================

void setup() {
  // Serial.begin(9600);
  // while (!Serial);

  // Motor
  pinMode(ENABLE_MOTOR, OUTPUT);
  pinMode(MOTORA1, OUTPUT);
  pinMode(MOTORA2, OUTPUT);

  // RGB LED
  WiFiDrv::pinMode(25, OUTPUT);
  WiFiDrv::pinMode(26, OUTPUT);
  WiFiDrv::pinMode(27, OUTPUT);

  // RTC
  rtc.begin();
  rtc.setHours(0);
  rtc.setMinutes(0);
  rtc.setSeconds(0);

  // Pressure sensor
  if (!mpr.begin()) {
    setLED(255, 0, 0); // red = failure
    while (1);
  }
  setLED(0, 255, 0); // green = OK

  // WiFi AP
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    while (true);
  }

  delay(3000);
  server.begin();

  Serial.println("System Ready");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  handleWiFi();

  updatePump();

  switch (state) {
    case 1:
      runVerticalProfile();
      break;

    case 2:
      sendData();
      state = 0;
      break;

    case 3:
      startPump(85000, true);
      setLED(0, 0, 255);
      state = 0;
      break;

    case 4:
      sendReply();
      state = 0;
      break;

    case 5:
      if (!pumpRunning) {
        startPump(5000, true);
        setLED(0, 255, 255);
      }
      state = 0;
      break;

    case 6:
      if (!pumpRunning) {
        startPump(5000, false);
        setLED(255, 255, 0);
      }
      state = 0;
      break;
      }
}

// =====================================================
// WIFI CONTROL
// =====================================================

void handleWiFi() {
  WiFiClient client = server.available();

  if (!client) return;

  String line = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();

      if (c == '\n') {

        if (line.length() == 0) {

          // ===== RESPONSE =====
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          client.println("<html><head>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
          client.println("</head><body style='text-align:center;'>");

          client.println("<h1>ROV Control</h1>");

          client.println("<a href='/start'><button style='width:80%;height:60px;font-size:20px;'>Start Profile</button></a><br><br>");
          client.println("<a href='/fill'><button style='width:80%;height:60px;font-size:20px;'>Fill Neutral</button></a><br><br>");
          client.println("<a href='/test'><button style='width:80%;height:60px;font-size:20px;'>Test</button></a><br><br>");
          client.println("<a href='/in5'><button style='width:80%;height:60px;font-size:20px;'>Pump IN (5s)</button></a><br><br>");
          client.println("<a href='/out5'><button style='width:80%;height:60px;font-size:20px;'>Pump OUT (5s)</button></a><br><br>");

          client.print("<p>Depth: ");
          client.print(readPressure());
          client.println("</p>");

          client.println("</body></html>");
          break;
        } else {
          line = "";
        }
      }
      else if (c != '\r') {
        line += c;
      }

      // ===== COMMANDS =====
      if (line.endsWith("GET /start")) state = 1;
      if (line.endsWith("GET /fill"))  state = 3;
      if (line.endsWith("GET /test"))  state = 4;
      if (line.endsWith("GET /in5"))  state = 5;
      if (line.endsWith("GET /out5")) state = 6;
    }
  }

  client.stop();
}

// =====================================================
// PUMP CONTROL (NON-BLOCKING)
// =====================================================

void startPump(unsigned long duration, bool in) {
  pumpRunning = true;
  pumpDirectionIn = in;
  pumpDuration = duration;
  pumpStartTime = millis();

  digitalWrite(ENABLE_MOTOR, HIGH);

  if (in) {
    digitalWrite(MOTORA1, LOW);
    digitalWrite(MOTORA2, HIGH);
  } else {
    digitalWrite(MOTORA1, HIGH);
    digitalWrite(MOTORA2, LOW);
  }
}

void updatePump() {
  if (!pumpRunning) return;

  if (millis() - pumpStartTime >= pumpDuration) {
    digitalWrite(ENABLE_MOTOR, LOW);
    pumpRunning = false;
  }
}

// =====================================================
// VERTICAL PROFILE (NON-BLOCKING)
// =====================================================

void runVerticalProfile() {

  if (pumpRunning) return; // wait for pump to finish

  switch (profileStep) {

    case 0:
      Serial.println("Start Profile");
      setLED(255, 0, 0);
      startPump(5000, true);
      savePacket();
      profileStep++;
      break;

    case 1:
      startPump(5000, true);
      savePacket();
      profileStep++;
      break;

    case 2:
      stepStartTime = millis();
      profileStep++;
      break;

    case 3:
      if (millis() - stepStartTime > 5000) {
        savePacket();
        profileStep++;
      }
      break;

    case 4:
      startPump(5000, false);
      savePacket();
      profileStep++;
      break;

    case 5:
      stepStartTime = millis();
      profileStep++;
      break;

    case 6:
      if (millis() - stepStartTime > 5000) {
        savePacket();
        profileStep++;
      }
      break;

    case 7:
      state = 2;
      profileStep = 0;
      Serial.println("Profile Done");
      break;
  }
}

// =====================================================
// DATA + SENSORS
// =====================================================

float readPressure() {
  return mpr.readPressure();
}

void savePacket() {
  packetTime[packetCounter][0] = rtc.getMinutes();
  packetTime[packetCounter][1] = rtc.getSeconds();
  packetDepth[packetCounter] = (int) readPressure();
  packetCounter++;
}

// =====================================================
// OUTPUT (SERIAL INSTEAD OF LORA)
// =====================================================

void sendReply() {
  Serial.print("TEST_");
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  Serial.println(rtc.getSeconds());
}

void sendData() {
  for (int i = 0; i < packetCounter; i++) {
    Serial.print(TEAM_ID);
    Serial.print("_");
    Serial.print(packetTime[i][0]);
    Serial.print(":");
    Serial.print(packetTime[i][1]);
    Serial.print("_");
    Serial.println(packetDepth[i]);
  }
  packetCounter = 0;
}

// =====================================================
// LED
// =====================================================

void setLED(int r, int g, int b) {
  WiFiDrv::analogWrite(25, g);
  WiFiDrv::analogWrite(26, r);
  WiFiDrv::analogWrite(27, b);
}