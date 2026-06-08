// ============================================================================
// CHAMBRE FROIDE - Version ULTRA LÉGÈRE pour Arduino Uno
// ============================================================================

#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(2, DHT22);

const int FAN_PWM_PIN = 3;

float desiredTemp = 4.0;
float temp = -999.0;
float hum = -999.0;
int fanPWM = 0;
char currentChambreId[20] = "NONE";
bool sensorValid = false;

unsigned long lastRead = 0;
unsigned long lastDisplayDraw = 0;
bool telemetryDirty = true;
char inputBuffer[64];
uint8_t bufferIndex = 0;

void processCommand(char *cmd);
void updateFanControl();
void sendStatus();
void logDebug(const __FlashStringHelper *message);
void logDebugValue(const __FlashStringHelper *label, float value);

// Memory check
int freeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// Buffer global pour éviter les allocations dynamiques et crash de pile
char valBuffer[10];
unsigned int displayFrames = 0;

// ====================== ENVOI I2C SÉCURISÉ (100kHz) ======================
// Contourne display.display() car Adafruit force 400kHz ce qui fait planter
// les câbles longs.
void pushBufferSlow() {
  Wire.setClock(100000);
  uint8_t *ptr = display.getBuffer();
  for (uint8_t page = 0; page < 8; page++) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x00);
    Wire.write(0xB0 | page);
    Wire.write(0x00);
    Wire.write(0x10);
    Wire.endTransmission();

    for (uint8_t col = 0; col < 128; col += 16) {
      Wire.beginTransmission(OLED_ADDR);
      Wire.write(0x40);
      for (uint8_t j = 0; j < 16; j++) {
        Wire.write(ptr[page * 128 + col + j]);
      }
      Wire.endTransmission();
    }
  }
}

// ====================== AFFICHAGE NORMAL (léger) ======================
void drawNormal() {
  displayFrames++;
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  // --- Ligne 0 : ID ---
  display.setCursor(0, 0);
  display.print(F("ID:"));
  display.print(currentChambreId);

  display.setCursor(100, 0);
  display.print(displayFrames);

  display.drawFastHLine(0, 10, 128, SH110X_WHITE);

  // --- Température ---
  display.setCursor(0, 14);
  display.print(F("TEMP: "));
  if (sensorValid) {
    display.print((int)temp);
    display.print(F(" C"));
  } else {
    display.print(F("-- C"));
  }

  // --- Humidité ---
  display.setCursor(0, 26);
  display.print(F("HUMI: "));
  if (sensorValid) {
    display.print((int)hum);
    display.print(F(" %"));
  } else {
    display.print(F("-- %"));
  }

  // --- Ventilateur ---
  display.setCursor(0, 38);
  display.print(F("VENT: "));
  display.print((fanPWM * 100) / 255);
  display.print(F(" %"));

  // --- Consigne ---
  display.setCursor(0, 50);
  display.print(F("CONS: "));
  display.print((int)desiredTemp);
  display.print(F(" C"));

  pushBufferSlow();
}

// ====================== ANIMATION LÉGÈRE (Désactivée) ======================
void updateFanControl() {
  if (sensorValid && temp > desiredTemp) {
    long t10 = (long)(temp * 10.0);
    long sp10 = (long)(desiredTemp * 10.0);
    long sp510 = (long)((desiredTemp + 5.0) * 10.0);
    fanPWM = (int)map(t10, sp10, sp510, 50L, 255L);
    fanPWM = constrain(fanPWM, 50, 255);
  } else {
    fanPWM = 0;
  }

  // analogWrite(FAN_PWM_PIN, fanPWM); // DESACTIVE POUR TEST MATERIEL
}

void sendStatus() {
  Serial.print(F("STATUS;TEMP="));
  if (sensorValid)
    Serial.print(temp, 2);
  else
    Serial.print(F("nan"));
  Serial.print(F(";HUMIDITY="));
  if (sensorValid)
    Serial.print(hum, 2);
  else
    Serial.print(F("nan"));
  Serial.print(F(";SETPOINT="));
  Serial.print(desiredTemp, 2);
  Serial.print(F(";FAN_PWM="));
  Serial.println(fanPWM);
}

void logDebug(const __FlashStringHelper *message) {
  Serial.print(F("DBG;"));
  Serial.println(message);
}

void logDebugValue(const __FlashStringHelper *label, float value) {
  Serial.print(F("DBG;"));
  Serial.print(label);
  Serial.println(value, 2);
}

// ====================== SETUP ======================
void setup() {
  pinMode(FAN_PWM_PIN, OUTPUT);
  // analogWrite(FAN_PWM_PIN, 100); // DESACTIVE POUR TEST MATERIEL

  Serial.begin(9600);

  dht.begin();

  // Initialisation I2C standard
  Wire.begin();

  if (display.begin(OLED_ADDR, true)) {
    delay(100);
    display.setContrast(128);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 20);
    display.print(F("SYNC..."));
    pushBufferSlow();
  } else {
    Serial.println(F("OLED FAIL"));
  }

  // FORCE I2C à 100kHz ICI
  Wire.setClock(100000);

  logDebug(F("BOOT_OK"));
  logDebugValue(F("FREE_RAM="), freeMemory());

  lastRead = millis();
  lastDisplayDraw = millis();
  // On ne fait plus de drawNormal() ici pour éviter un double appel rapide avec
  // le loop
}

// ====================== LOOP ======================
void loop() {
  unsigned long now = millis();

  // Check for Serial commands
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufferIndex > 0) {
        inputBuffer[bufferIndex] = '\0';
        processCommand(inputBuffer);
        bufferIndex = 0;
      }
    } else if (bufferIndex < (sizeof(inputBuffer) - 1)) {
      inputBuffer[bufferIndex++] = c;
    }
  }

  // Lecture DHT22
  if (now - lastRead >= 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      sensorValid = true;
      temp = t;
      hum = h;

      updateFanControl();

      // Mise à jour de l'écran après chaque lecture réussie
      telemetryDirty = true;
    } else {
      sensorValid = false;
      logDebug(F("DHT_READ_FAIL"));
    }

    lastRead = now;
  }

  // Gestion de l'affichage sécurisée
  if (telemetryDirty && (now - lastDisplayDraw >= 1000)) {
    logDebugValue(F("DRAW_TEMP="), temp);
    drawNormal();
    telemetryDirty = false;
    lastDisplayDraw = now;
  }
}

void processCommand(char *cmd) {
  // Trim trailing spaces/CR
  int len = strlen(cmd);
  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r')) {
    cmd[--len] = '\0';
  }

  Serial.print(F("DBG;RX_CMD="));
  Serial.println(cmd);
  bool forceRefresh = false;

  if (strncmp(cmd, "SETPOINT=", 9) == 0) {
    float newSetpoint = atof(cmd + 9);
    if (newSetpoint >= -30.0 && newSetpoint <= 30.0) {
      desiredTemp = newSetpoint;
      logDebugValue(F("SETPOINT_APPLIED="), desiredTemp);
      updateFanControl();
      forceRefresh = true;
    } else {
      logDebug(F("SETPOINT_OUT_OF_RANGE"));
    }
  } else if (strncmp(cmd, "ID=", 3) == 0) {
    strncpy(currentChambreId, cmd + 3, sizeof(currentChambreId) - 1);
    currentChambreId[sizeof(currentChambreId) - 1] = '\0';
    logDebug(F("ID_UPDATED"));
    forceRefresh = true;
  } else if (strcmp(cmd, "POLL") == 0) {
    sendStatus();
  } else {
    logDebug(F("CMD_UNKNOWN"));
  }

  if (forceRefresh) {
    unsigned long t = millis();
    lastDisplayDraw = t;
    telemetryDirty = false;
    drawNormal(); // Affichage immédiat, pas de dépendance au timing du loop
  }
}
