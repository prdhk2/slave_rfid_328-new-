#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

// Pin configuration for RS485
#define enTxPin 8   // HIGH: TX, LOW: RX
SoftwareSerial mySerial(2, 3); // RX, TX (using non-SPI pins)

// Pin configuration for RFID RC522
#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Data block configuration
const byte blocks[] = {4, 5, 6, 8, 9};
const byte total_blocks = sizeof(blocks)/sizeof(blocks[0]);

// Data variables
int id_driver = 0;
String nama_driver = "";
int id_customer = 0;
String no_polisi = "";
String nama_customer = "";

void checkRFIDPresence();
void handleReadingRFIDState();
bool readRFIDData();
void sendDataViaRS485();
void resetRFIDData(bool resetHardware);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  mySerial.begin(9600);
  pinMode(enTxPin, OUTPUT);
  digitalWrite(enTxPin, LOW); // Receive mode
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize default key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  Serial.println(F("RFID Reader Ready"));
}

void loop() {
  // Check for new RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (readRFIDData()) {
      sendDataViaRS485();
    }
    resetRFIDData(false); // Prepare for next card
  }
  delay(100); // Small delay to prevent bus saturation
}

bool readRFIDData() {
    char temp_id_driver[17] = "";
    char temp_nama_driver[17] = "";
    char temp_id_customer[17] = "";
    char temp_no_polisi[17] = "";
    char temp_nama_customer[17] = "";

    for (byte i = 0; i < total_blocks; i++) {
        byte buffer[18] = {0};
        byte block = blocks[i];
        byte len = 18;

        // Authenticate and read block
        if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
            Serial.print(F("Auth failed block ")); Serial.println(block);
            continue;
        }

        if (mfrc522.MIFARE_Read(block, buffer, &len) != MFRC522::STATUS_OK) {
            Serial.print(F("Read failed block ")); Serial.println(block);
            continue;
        }

        buffer[16] = 0; // Null terminator
        switch (block) {
            case 4: strncpy(temp_id_driver, (char*)buffer, 16); break;
            case 5: strncpy(temp_nama_driver, (char*)buffer, 16); break;
            case 6: strncpy(temp_id_customer, (char*)buffer, 16); break;
            case 8: strncpy(temp_no_polisi, (char*)buffer, 16); break;
            case 9: strncpy(temp_nama_customer, (char*)buffer, 16); break;
        }
    }

    // Validate data
    if (strlen(temp_id_driver) == 0 || strlen(temp_nama_driver) == 0 ||
        strlen(temp_id_customer) == 0 || strlen(temp_no_polisi) == 0 ||
        strlen(temp_nama_customer) == 0) {
        Serial.println(F("Incomplete RFID data"));
        return false;
    }

    // Store data
    id_driver = atoi(temp_id_driver);
    nama_driver = String(temp_nama_driver); nama_driver.trim();
    id_customer = atoi(temp_id_customer);
    no_polisi = String(temp_no_polisi); no_polisi.trim();
    nama_customer = String(temp_nama_customer); nama_customer.trim();

    return true;
}

void sendDataViaRS485() {
    digitalWrite(enTxPin, HIGH); // Enable transmitter
    
    // Format: IDDriver|NamaDriver|IDCustomer|NoPolisi|NamaCustomer
    String data = String(id_driver) + "," + 
                 nama_driver + "," + 
                 String(id_customer) + "," + 
                 no_polisi + "," + 
                 nama_customer;
    
    // Send with framing
    mySerial.write(0x02); // STX
    mySerial.print(data);
    mySerial.write(0x03); // ETX
    
    mySerial.flush();
    digitalWrite(enTxPin, LOW); // Disable transmitter
    
    Serial.print(F("Sent: ")); Serial.println(data);
}

void resetRFIDData(bool hardReset) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    if (hardReset) {
        mfrc522.PCD_Init();
        Serial.println(F("RFID hardware reset"));
    }
}