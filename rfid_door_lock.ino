#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#define SS_PIN      21
#define RST_PIN     22
#define SERVO_PIN   13
#define BUZZER_PIN  15 // Define the pin for the buzzer

// Servo positions (adjust for your flap)
const int LOCK_POS    = 0;   // Horn blocks the door
const int UNLOCK_POS = 90;   // Horn moves away
const uint32_t UNLOCK_TIME_MS = 7000; // Unlocked duration

// Authorized UID updated with your new card
byte authorizedUID[4] = {0xB7, 0x7E, 0xD5, 0x05};

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo lockServo;

bool checkUID(byte *readUID) {
  for (byte i = 0; i < 4; i++) {
    if (readUID[i] != authorizedUID[i]) return false;
  }
  return true;
}

void setLocked(bool locked) {
  lockServo.write(locked ? LOCK_POS : UNLOCK_POS);
}

// New function to slowly move the servo to the unlock position
void unlockSlowly() {
  for (int pos = LOCK_POS; pos <= UNLOCK_POS; pos += 1) { // Moves 1 degree at a time
    lockServo.write(pos);
    delay(15); // Adjust this delay to change the speed (higher value = slower)
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize the buzzer pin as an output
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  lockServo.setPeriodHertz(50); // Servo frequency
  lockServo.attach(SERVO_PIN, 500, 2500);

  setLocked(true); // Start locked
  Serial.println("Smart RFID Lock Ready. Present your card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  byte readUID[4];
  Serial.print("Scanned UID: ");
  for (byte i = 0; i < 4; i++) {
    readUID[i] = mfrc522.uid.uidByte[i];
    Serial.print(readUID[i], HEX); Serial.print(" ");
  }
  Serial.println();

  if (checkUID(readUID)) {
    Serial.println("Access Granted ✅ Unlocking slowly...");
    unlockSlowly(); // Use the new slow unlock function
    delay(UNLOCK_TIME_MS);
    Serial.println("Relocking...");
    setLocked(true); // Keep the fast lock
  } else {
    Serial.println("Access Denied ❌");
    // --- Buzzer logic for denied access ---
    Serial.println("Sounding alarm...");
    digitalWrite(BUZZER_PIN, HIGH); // Turn buzzer on
    delay(3000);                    // Keep it on for 3 seconds
    digitalWrite(BUZZER_PIN, LOW);  // Turn buzzer off
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}