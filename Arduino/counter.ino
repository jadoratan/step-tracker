int i = 0;

void setup() {
  // put your setup code here, to run once:
  // Wire1.begin(22, 19);
  Serial.begin(115200);
  // while (!Serial) delay(10);
  // Serial.println("Scanning I2C bus (Wire1 / STEMMA QT)...");
}

void loop() {
  // put your main code here, to run repeatedly:
  i++;
  Serial.print("Current step: ");
  Serial.println(i);
  delay(200);
}
