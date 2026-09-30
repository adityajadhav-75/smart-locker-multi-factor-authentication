#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fingerprint
SoftwareSerial mySerial(3, 2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// RELAY (ACTIVE LOW)
#define RELAY_PIN 8

// BUZZER ON RX PIN
#define BUZZER_PIN 1

// FAILED ATTEMPTS
int failedAttempts = 0;

// Keypad
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {A0, A1, A2, A3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// USER DATA
String userRFID[3] = {
  "USER1_UID",
  "USER2_UID",
  "USER3_UID"
};

String userPIN[3] = {
  "PIN1",
  "PIN2",
  "PIN3"
};

int userFinger[3] = {
  1,
  2,
  3
};

// MASTER ACCESS
String masterUID = "MASTER_UID";
String masterPIN = "MASTER_PIN"

void setup() {

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  // LCD
  lcd.init();
  lcd.backlight();

  // RELAY
  pinMode(RELAY_PIN, OUTPUT);

  // LOCKED STATE
  digitalWrite(RELAY_PIN, HIGH);

  // BUZZER
  pinMode(BUZZER_PIN, OUTPUT);

  // BUZZER OFF
  digitalWrite(BUZZER_PIN, LOW);

  // Fingerprint
  finger.begin(57600);

  if (!finger.verifyPassword()) {

    lcd.clear();
    lcd.print("Finger Error");

    while (1);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SMART LOCKER");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  delay(2000);
}

void loop() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SCAN RFID CARD");

  // WAIT FOR CARD
  if (!rfid.PICC_IsNewCardPresent() ||
      !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // READ UID
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();

  // =========================
  // MASTER CARD ACCESS
  // =========================
  if (uid == masterUID) {

    lcd.clear();
    lcd.print("MASTER CARD");

    delay(1000);

    lcd.clear();
    lcd.print("ENTER M PIN:");

    String enteredPIN = "";

    while (enteredPIN.length() < 4) {

      char key = keypad.getKey();

      if (key) {

        enteredPIN += key;

        lcd.setCursor(enteredPIN.length() - 1, 1);
        lcd.print("*");
      }
    }

    // MASTER PIN CHECK
    if (enteredPIN == masterPIN) {

      failedAttempts = 0;

      lcd.clear();
      lcd.print("MASTER ACCESS");

      lcd.setCursor(0, 1);
      lcd.print("GRANTED");

      unlockLocker();

    } else {

      failedAttempts++;

      lcd.clear();
      lcd.print("WRONG M PIN");

      checkSecurityAlert();

      delay(2000);
    }

    return;
  }

  // =========================
  // NORMAL USER CHECK
  // =========================
  int userIndex = getUserFromRFID(uid);

  // INVALID RFID
  if (userIndex == -1) {

    failedAttempts++;

    accessDenied("RFID FAILED");

    checkSecurityAlert();

    return;
  }

  // RFID OK
  lcd.clear();
  lcd.print("RFID VERIFIED");

  delay(1000);

  // PIN CHECK
  if (!checkPIN(userIndex)) {

    failedAttempts++;

    accessDenied("WRONG PIN");

    checkSecurityAlert();

    return;
  }

  lcd.clear();
  lcd.print("PIN VERIFIED");

  delay(1000);

  // FINGERPRINT CHECK
  if (!checkFingerprint(userIndex)) {

    failedAttempts++;

    accessDenied("FINGER FAILED");

    checkSecurityAlert();

    return;
  }

  // ACCESS GRANTED
  failedAttempts = 0;

  lcd.clear();
  lcd.print("ACCESS GRANTED");

  unlockLocker();
}

// ======================================
// UNLOCK LOCKER
// ======================================
void unlockLocker() {

  // UNLOCK
  digitalWrite(RELAY_PIN, LOW);

  delay(10000);

  // LOCK AGAIN
  digitalWrite(RELAY_PIN, HIGH);

  lcd.clear();
  lcd.print("LOCKED AGAIN");

  delay(2000);
}

// ======================================
// RFID CHECK
// ======================================
int getUserFromRFID(String uid) {

  for (int i = 0; i < 3; i++) {

    if (uid == userRFID[i]) {
      return i;
    }
  }

  return -1;
}

// ======================================
// PIN CHECK
// ======================================
bool checkPIN(int userIndex) {

  lcd.clear();
  lcd.print("ENTER PIN:");

  String entered = "";

  while (entered.length() < 4) {

    char key = keypad.getKey();

    if (key) {

      entered += key;

      lcd.setCursor(entered.length() - 1, 1);
      lcd.print("*");
    }
  }

  return (entered == userPIN[userIndex]);
}

// ======================================
// FINGERPRINT CHECK
// ======================================
bool checkFingerprint(int userIndex) {

  lcd.clear();
  lcd.print("PLACE FINGER");

  while (true) {

    uint8_t p = finger.getImage();

    if (p == FINGERPRINT_OK) {
      break;
    }
  }

  uint8_t p = finger.image2Tz();

  if (p != FINGERPRINT_OK) {
    return false;
  }

  p = finger.fingerFastSearch();

  if (p != FINGERPRINT_OK) {
    return false;
  }

  int id = finger.fingerID;

  return (id == userFinger[userIndex]);
}

// ======================================
// ACCESS DENIED
// ======================================
void accessDenied(String reason) {

  lcd.clear();

  lcd.print("ACCESS DENIED");

  lcd.setCursor(0, 1);
  lcd.print(reason);

  delay(2000);
}

// ======================================
// SECURITY ALERT
// ======================================
void checkSecurityAlert() {

  if (failedAttempts >= 3) {

    lcd.clear();
    lcd.print("SECURITY ALERT");

    lcd.setCursor(0, 1);
    lcd.print("BUZZER ON");

    // BUZZER ON
    digitalWrite(BUZZER_PIN, HIGH);

    delay(10000);

    // BUZZER OFF
    digitalWrite(BUZZER_PIN, LOW);

    failedAttempts = 0;
  }
}
