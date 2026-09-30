#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(3, 2); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int id = 1;

void setup() {
  Serial.begin(9600);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("Sensor not found");
    while (1);
  }
}

void loop() {
  if (id <= 3) {
    Serial.print("Enrolling Finger ID: ");
    Serial.println(id);

    enrollFinger(id);
    id++;

    delay(2000);
  } else {
    Serial.println("All 3 fingerprints enrolled!");
    while (1);
  }
}

void enrollFinger(int id) {
  int p = -1;

  Serial.println("Place finger...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  finger.image2Tz(1);

  Serial.println("Remove finger...");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Place same finger again...");
  while (finger.getImage() != FINGERPRINT_OK);

  finger.image2Tz(2);

  if (finger.createModel() == FINGERPRINT_OK) {
    if (finger.storeModel(id) == FINGERPRINT_OK) {
      Serial.println("Stored successfully!");
    } else {
      Serial.println("Error storing");
    }
  } else {
    Serial.println("Error creating model");
  }
}