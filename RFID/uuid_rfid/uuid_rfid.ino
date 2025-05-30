#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D4  // GPIO2 (Slave Select)
#define RST_PIN D3 // GPIO0 (Reset)

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

// Define the MIFARE key (default key for new cards)
MFRC522::MIFARE_Key key = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}; // Initialize keyByte array

void setup() {
  Serial.begin(115200); // Initialize serial communication
  while (!Serial);      // Wait for Serial to initialize
  SPI.begin();          // Initialize SPI bus
  rfid.PCD_Init();      // Initialize MFRC522
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

void loop() {
  // Reset the loop if no new card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verify if the card's UID has been read
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Check if the card is MIFARE Classic
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("Error: Card is not MIFARE Classic");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // Print card type
  Serial.print("RFID/NFC Tag Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Print UID in hexadecimal format
  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Write "hello" to block 4
  if (writeHelloWorldToCard()) {
    Serial.println("Write successful");
  } else {
    Serial.println("Write failed");
  }

  // Halt the card and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Delay to prevent rapid re-scans
  delay(1000); // Wait 1 second before next scan
}

bool writeHelloWorldToCard() {
  // Authenticate the sector (block 4 is in sector 1)
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return false;
  }

  // Prepare data: "hello" as bytes, padded with zeros
  String test = "hello";
  byte stringByte[16] = {0};
  for (int i = 0; i < test.length() && i < 16; i++) {
    stringByte[i] = test[i];
  }

  // Write to block 4
  status = rfid.MIFARE_Write(4, stringByte, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return false;
  }

  return true;
}