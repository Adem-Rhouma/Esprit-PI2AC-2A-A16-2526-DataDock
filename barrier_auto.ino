#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 7
#define SERVO_PIN 9
#define TRIG_PIN 5  
#define ECHO_PIN 6  
#define RED_LED 2    
#define BLUE_LED 3   
#define YELLOW_LED 4 

MFRC522 rfid(SS_PIN, RST_PIN); 
Servo barrierServo;

void setup() { 
  Serial.begin(9600);
  while (!Serial); 
  
  SPI.begin(); 
  initReader();

  barrierServo.attach(SERVO_PIN);
  barrierServo.write(0); 

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
}

void initReader() {
  SPI.end(); 
  delay(100);
  SPI.begin();
  
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(200); 
  digitalWrite(RST_PIN, HIGH);
  delay(200);

  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max); 
  rfid.PCD_AntennaOn(); 
}

long getDistance() {
  long duration;
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(15);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH, 30000); 
    if (duration > 0) break;
    delay(10);
  }
  return duration * 0.034 / 2;
}

void setLEDs(bool red, bool blue, bool yellow) {
  digitalWrite(RED_LED, red);
  digitalWrite(BLUE_LED, blue);
  digitalWrite(YELLOW_LED, yellow);
}
 
void loop() {
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    setLEDs(HIGH, LOW, LOW); 
    initReader();
    delay(1000); 
    return;
  }

  setLEDs(LOW, LOW, HIGH); 

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
 
  // ONLY OUTPUT THE RFID HEX CODE
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println(); 

  // Wait for 1 or 0 from PC
  unsigned long waitStart = millis();
  while (Serial.available() == 0) {
    if (millis() - waitStart > 10000) return; 
    delay(10); 
  }
  char response = Serial.read();

  if (response == 'a') {
    setLEDs(LOW, HIGH, LOW); 
    barrierServo.write(90); 
    
    unsigned long openTime = millis();
    while(millis() - openTime < 5000) { 
      if (getDistance() < 10) {
        while(getDistance() < 10) { delay(50); } 
        break;
      }
      delay(50);
    }
    delay(500); 
    barrierServo.write(0); 
  } 
  else {
    setLEDs(HIGH, LOW, LOW); 
    delay(2000);
  }

  rfid.PICC_HaltA(); 
  rfid.PCD_StopCrypto1();
}
