const int ROWS = 4;
const int COLS = 4;

// Clavier matriciel
char keys[ROWS][COLS] = {
  {'D','C','B','A'},
  {'#','9','6','3'},
  {'0','8','5','2'},
  {'*','7','4','1'}
};

byte pin_rows[ROWS] = {11, 10, 9, 8};
byte pin_column[COLS] = {7, 6, 5, 4};

// Eléments
const int buzzerPin = 2;
const int redLedPin = 3;
const int blueLedPin = 12;   // déplacée de 13 vers 12
const int buttonPin = 13;    // bouton sur 13

char data;
int voitEmpl = 0;

// pour détecter l'appui une seule fois
int lastButtonState = HIGH;

void setup() {
  Serial.begin(9600);

  // Configuration clavier
  for (int i = 0; i < ROWS; i++) {
    pinMode(pin_rows[i], INPUT_PULLUP);
  }

  for (int i = 0; i < COLS; i++) {
    pinMode(pin_column[i], OUTPUT);
    digitalWrite(pin_column[i], HIGH);
  }

  // LEDs + buzzer
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);

  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // bouton
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  // =========================
  // 1) Gestion du bouton
  // =========================
  int buttonState = digitalRead(buttonPin);

  // si le bouton vient d'être appuyé
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.write('e');   // envoyer e
    delay(50);           // petit anti-rebond
  }

  lastButtonState = buttonState;

  // =========================
  // 2) Gestion du clavier
  // =========================
  char key = getKey();

  if (key != 0) {
    if (key == 'D') {
      eteindreTout();
    }
    else if (key == 'A') {
      if (voitEmpl == 0) {
        voitEmpl = 1;
        signalModeVoiture();
      } else {
        voitEmpl = 0;
        signalModeCours();
      }
    }
    else {
      Serial.write(key);
    }
  }

  // =========================
  // 3) Gestion des messages reçus
  // =========================
  if (Serial.available()) {
    data = Serial.read();

    if (data == '1') {
      verificationEchouee();
    }
    else if (data == '2') {
      verificationValidee();
    }
    else if (data == '3') {
      enregistrementBDDOK();
    }
    else if (data == '4') {
      enregistrementBDDEchec();
    }
  }
}

char getKey() {
  for (int col = 0; col < COLS; col++) {
    digitalWrite(pin_column[col], LOW);

    for (int row = 0; row < ROWS; row++) {
      if (digitalRead(pin_rows[row]) == LOW) {
        delay(50);
        while (digitalRead(pin_rows[row]) == LOW);
        digitalWrite(pin_column[col], HIGH);
        return keys[row][col];
      }
    }

    digitalWrite(pin_column[col], HIGH);
  }

  return 0;
}

void eteindreTout() {
  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, LOW);
  noTone(buzzerPin);
}

void bipCourt() {
  tone(buzzerPin, 1000);
  delay(150);
  noTone(buzzerPin);
}

void verificationEchouee() {
  eteindreTout();

  for (int i = 0; i < 3; i++) {
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 800);
    delay(200);
    digitalWrite(redLedPin, LOW);
    noTone(buzzerPin);
    delay(150);
  }
}

void verificationValidee() {
  eteindreTout();

  digitalWrite(blueLedPin, HIGH);
  bipCourt();
  delay(800);
  digitalWrite(blueLedPin, LOW);
}

void enregistrementBDDOK() {
  eteindreTout();

  for (int i = 0; i < 2; i++) {
    digitalWrite(blueLedPin, HIGH);
    delay(200);
    digitalWrite(blueLedPin, LOW);
    delay(200);
  }

  bipCourt();
}

void enregistrementBDDEchec() {
  eteindreTout();

  for (int i = 0; i < 2; i++) {
    digitalWrite(redLedPin, HIGH);
    tone(buzzerPin, 600);
    delay(300);
    digitalWrite(redLedPin, LOW);
    noTone(buzzerPin);
    delay(200);
  }
}

void signalModeVoiture() {
  eteindreTout();
  digitalWrite(blueLedPin, HIGH);
  bipCourt();
  delay(300);
  digitalWrite(blueLedPin, LOW);
}

void signalModeCours() {
  eteindreTout();
  digitalWrite(redLedPin, HIGH);
  bipCourt();
  delay(300);
  digitalWrite(redLedPin, LOW);
}