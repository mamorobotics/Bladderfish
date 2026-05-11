int ENABLE = 2;
int MotorA1 = 3;
int MotorA2 = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ENABLE, OUTPUT);
  pinMode(MotorA1, OUTPUT);
  pinMode(MotorA2, OUTPUT);
  Serial.println("STARTING");

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(ENABLE, HIGH);
  digitalWrite(MotorA1, HIGH);
  digitalWrite(MotorA2, LOW);
  Serial.println("FORWARDS");

  delay(5000);

  digitalWrite(ENABLE, HIGH);
  digitalWrite(MotorA1, LOW);
  digitalWrite(MotorA2, HIGH);
  Serial.println("BACKWARDS");

  delay(5000);

  digitalWrite(ENABLE, LOW);
  Serial.println("STOP");
  delay(5000);

}
