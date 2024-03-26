
int OutputPin = 20;
int analogPin = 16;
int sampleVal = 0;
int sampleAmt = 1000;

void setup() {
  // put your setup code here, to run once:
  pinMode(OutputPin,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  digitalWrite(OutputPin,HIGH);
  delay(1000);
  for(int i = 0; i < sampleAmt; i++){
    int capLevel = analogRead(analogPin);
    float voltage = capLevel * (3.3/1024);
    Serial.println(voltage);
    digitalWrite(OutputPin,LOW);
  }
  exit(0);
}
