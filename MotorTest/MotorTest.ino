int ENABLE = 2;
int MotorA1 = 3;
int MotorA2 = 5;

String command = "STOP";

void setup() {
  Serial.begin(9600);
  pinMode(ENABLE, OUTPUT);
  pinMode(MotorA1, OUTPUT);
  pinMode(MotorA2, OUTPUT);
  Serial.println("READY: type ADD, REMOVE, or STOP");
}

void setPump(const String &cmd) {
  if (cmd == "ADD") {
    digitalWrite(ENABLE, HIGH);
    digitalWrite(MotorA1, LOW);
    digitalWrite(MotorA2, HIGH);
    Serial.println("ADD WATER");
  } else if (cmd == "REMOVE") {
    digitalWrite(ENABLE, HIGH);
    digitalWrite(MotorA1, HIGH);
    digitalWrite(MotorA2, LOW);
    Serial.println("REMOVE WATER");
  } else { // STOP or unknown
    digitalWrite(ENABLE, LOW);
    digitalWrite(MotorA1, LOW);
    digitalWrite(MotorA2, LOW);
    Serial.println("STOP");
  }
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    if (input.length() > 0) {
      command = input;
    }
  }

  setPump(command);
  delay(200); // small delay to avoid spamming Serial
}