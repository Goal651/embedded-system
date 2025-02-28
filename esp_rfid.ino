#include <AES.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>

#define SS_PIN 15       // D8
#define RST_PIN 16      // D0
#define AMBER_LED_PIN 4 // D2
#define GREEN_LED_PIN 2 // D4
#define BUZZER_PIN 5    // D1
#define NAME_BLOCK 5

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode card_status;
WiFiClient wifiClient;

const char *host = "iot.benax.rw";
void setup()
{
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(AMBER_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    Serial.begin(115200);
    connectToWiFi("EdNet", "Huawei@123");
    SPI.begin();
    mfrc522.PCD_Init();
}

void loop()
{
    byte block_number = 4;
    byte buffer_for_reading[18];

    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    Serial.println("Preparing for transaction");

    String currentBalance = readBalanceFromCard(4, buffer_for_reading);
    String owner = readOwnerName(5, buffer_for_reading);

    Serial.println("User is " + owner);
    Serial.println("Current balance is " + currentBalance);

    if (currentBalance.length() > 0)
    {
        float floatBalance = currentBalance.toFloat();
        String newBalanceStr = String(floatBalance);
        String newBalance = operateData(block_number, newBalanceStr);
        connectToHost(80);
        sendDataToServer(owner, floatBalance, 450);
    }
    else
    {
        Serial.println("Failed to read current balance!");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void connectToWiFi(const char *ssid, const char *passwd)
{
    WiFi.mode(WIFI_OFF); // This prevents reconnection issue
    delay(10);
    WiFi.mode(WIFI_STA);      // This hides the viewing of ESP as wifi hotspot
    WiFi.begin(ssid, passwd); // Connect to your WiFi router

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();
}

void connectToHost(const int httpPort)
{
    int retry_counter = 0;
    wifiClient.setTimeout(15000);
    delay(1000);
    Serial.printf("Connecting to \"%s\"\n", host);

    while ((!wifiClient.connect(host, httpPort)) && (retry_counter <= 30))
    {
        delay(100);
        Serial.print(".");
        retry_counter++;
    }

    if (retry_counter == 31)
    {
        Serial.println("\nConnection failed.");
        return;
    }
    else
    {
        Serial.printf("Connected to \"%s\"\n", host);
    }
}

void sendDataToServer(String cardID, float balance, float transportFare)
{
    if (!wifiClient.connect(host, 80))
    {
        Serial.println("Connection to server failed!");
        return;
    }

    String postData = "customer=" + cardID + "&initial_balance=" + balance + "&transport_fare=" + transportFare;

    wifiClient.println("POST /projects/b6bcd0d59507dc9220676be1236cdb8e/IOT_Backend/RFID/upload.php HTTP/1.1");
    wifiClient.println("Host: " + String(host));
    wifiClient.println("User-Agent: ESP8266/1.0");
    wifiClient.println("Content-Type: application/x-www-form-urlencoded");
    wifiClient.println("Content-Length: " + String(postData.length()));
    wifiClient.println();
    wifiClient.print(postData);

    delay(1000);
    getFeedback();
}

void getFeedback()
{
    String datarx = "";
    Serial.println("Waiting for server response...");

    // Read HTTP headers
    while (wifiClient.connected())
    {
        String line = wifiClient.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }

    while (wifiClient.available())
    {
        datarx += wifiClient.readString();
    }

    Serial.println("Response: " + datarx);

    wifiClient.stop();
}

String readBalanceFromCard(byte blockNumber, byte readingBuffer[])
{
    card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));

    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return "";
    }

    byte readDataLength = 18;
    card_status = mfrc522.MIFARE_Read(blockNumber, readingBuffer, &readDataLength);

    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return "";
    }

    String value = "";
    for (uint8_t i = 0; i < 16; i++)
    {
        if (readingBuffer[i] == 0x00)
            break;
        value += (char)readingBuffer[i];
    }
    value.trim();

    // Remove non-printable characters (e.g., 0x10 or others)
    value.replace(String((char)0x10), "");

    return value;
}

bool saveBalanceToCard(byte blockNumber, byte writingBuffer[])
{
    card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));

    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return false;
    }

    card_status = mfrc522.MIFARE_Write(blockNumber, writingBuffer, 16);

    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return false;
    }

    delay(3000);
    return true;
}

String operateData(byte blockNumber, String initialBalance)
{
    int transportFare = 450;
    float newBalance = initialBalance.toInt() - transportFare;

    if (initialBalance.toInt() < transportFare)
    {
        blinkLEDWhileBuzzing(AMBER_LED_PIN, BUZZER_PIN, 600, 400, 4);
        Serial.print("Insufficient Balance: ");
        Serial.println(initialBalance);
        return initialBalance;
    }

    String initial_balance_str = String(newBalance);
    char writingBuffer[16];
    initial_balance_str.toCharArray(writingBuffer, 16);

    saveBalanceToCard(blockNumber, (unsigned char *)writingBuffer);
    return initial_balance_str;
}

void blinkLEDWhileBuzzing(int LED_Pin, int Buzzer_Pin, int t0, int t1, int n)
{
    for (int i = 0; i < n; i++)
    {
        digitalWrite(LED_Pin, 1);
        digitalWrite(Buzzer_Pin, 1);
        delay(t0);
        digitalWrite(LED_Pin, 0);
        digitalWrite(Buzzer_Pin, 0);
        delay(t1);
    }
}

String getUUID()
{
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    return content;
}

void saveOwnerName(String ownerName)
{
    // Prepare a buffer for 16 bytes (or characters)
    char nameBuffer[16];
    // Convert the name to a char array
    ownerName.toCharArray(nameBuffer, 16);
    // Write to the card using the same function (it handles authentication and writing)
    if (saveBalanceToCard(5, (unsigned char *)nameBuffer))
    {
        Serial.println("Owner name saved successfully!");
    }
    else
    {
        Serial.println("Failed to save owner name.");
    }
}

String readOwnerName(byte blockNumber, byte readingBuffer[])
{
    // Authenticate for the specific block
    card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return "";
    }

    byte readDataLength = 18;
    card_status = mfrc522.MIFARE_Read(blockNumber, readingBuffer, &readDataLength);
    if (card_status != MFRC522::STATUS_OK)
    {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(card_status));
        return "";
    }

    String name = "";
    for (uint8_t i = 0; i < 16; i++)
    {
        if (readingBuffer[i] == 0x00)
            break;
        name += (char)readingBuffer[i];
    }
    name.trim();
    return name;
}
