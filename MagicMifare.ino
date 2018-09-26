/* First version written by Pelle Domela Sep 2018
 * 
 * Dependencies:
 * MFRC522 Arduino Library https://github.com/miguelbalboa/rfid
 * 
 * Wiring:
 * Connect RC522 to Arduino:
 * SDA -> Pin 10
 * SCK -> Pin 13
 * MOSI -> Pin 11
 * MISO -> Pin 12
 * IRQ -> Not connected
 * GND -> GND
 * RST -> Pin 9
 * 3.3v -> 3.3v
 * 
 */

#include <MFRC522.h>
#include <MFRC522Hack.h>

#include <SPI.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522Hack mfrc522hack(&mfrc522);

void setup() 
{
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

}

void reset();
void setUid();

void loop() 
{

  Serial.println("Approximate your card to the reader...\n");
  
  // Look for new cards
  while( !mfrc522.PICC_IsNewCardPresent()){delay(5);}
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}

  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  } 
  Serial.println();

  int option = 0;
  Serial.println("Plase enter an option:");
  Serial.println("1:  Factory Reset (Zerofill data blocks and reset trailers, doesn't change UID)");
  Serial.println("2:  Change UID");
  Serial.println("3:  Do nothing");

  while(!Serial.available()) {delay(5);}
  option = Serial.parseInt();
  
  switch(option) {
    case 1:
      reset();
      break;
    case 2:
      setUid();
      break;
    case 3:
      break;
    default:
      Serial.println("Please chose a valid opition!");
      return;
    
  }

  // This is here to let you keep the card on the reader,
  // otherwise you would have to remove it, and put it back again each time
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  mfrc522.PCD_SoftPowerDown();
  delay(500);
  mfrc522.PCD_SoftPowerUp();

  Serial.println();
  
}

void reset() {
  Serial.println("Resetting card!");

  MFRC522::StatusCode status;
  byte uid[4] = { 0xDE, 0xAD, 0xBE, 0xEF }; 

  // This opens the backdoor on the chineese cards, which lets you write to whatever block
  // you want, you dont even need to authenticate.
  mfrc522hack.MIFARE_OpenUidBackdoor(false);
  
  const int noOfSectors = 16;
  const int blocksPerSector = 4;
  const byte infill[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  const byte trailer[16] = { /*Key A*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                  /*Acess Conditions*/ 0xFF, 0x07, 0x80, 0x69,
                             /*Key B*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Sector 0 gets some special treatment, because of the manufacturer block
  status = mfrc522.MIFARE_Write(1, infill, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.println("Could not write to block 1");
    Serial.print("Reason: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Write(2, infill, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.println("Could not write to block 2");
    Serial.print("Reason: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Write(3, trailer, 16);
  if(status != MFRC522::STATUS_OK) {
    Serial.println("Could not write to block 3");
    Serial.print("Reason: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Loop through the rest of the blocks, clear them and reset their trailer (keys and access conditions)
  for(int sector = 1; sector < noOfSectors; sector++) {
    for(int block = 0; block < blocksPerSector - 1; block++) { //skip trailer block

      status = mfrc522.MIFARE_Write(sector*blocksPerSector+block, infill, 16);
      if(status != MFRC522::STATUS_OK) {
        Serial.println("Could not write to block " + String(sector*blocksPerSector+block));
        Serial.print("Reason: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      
    }

    status = mfrc522.MIFARE_Write(sector*blocksPerSector+3, trailer, 16);
    if(status != MFRC522::STATUS_OK) {
      Serial.println("Could not write to block " + String(sector*blocksPerSector+3));
      Serial.print("Reason: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    //Status indicator, so you can see that something is happening!
    Serial.print(".");
  }
  Serial.println();
  
  Serial.println("Done!");
  
}

void setUid() {
  Serial.println("Please enter new UID (example: DEADBEEF)");
  while(!(Serial.available() >= 8)) {}
  String input = Serial.readString();
  input.toUpperCase();

  byte uid[4] = { 0x00, 0x00, 0x00, 0x00};

  // This for loops parses the hex string input, and stores it as bytes in the uid array
  int index = 0;
  for(int bt = 0; bt < 4; bt++) {
    byte nl = (byte)input.charAt(bt*2);
    byte nr = (byte)input.charAt(bt*2+1);
    if(nl > 64) {
      uid[index] += (nl-55)*16;
    } else {
      uid[index] += (nl-48)*16;
    }

    if(nr > 64) {
      uid[index] += nr-55;
    } else {
      uid[index] += nr-48;
    }
    index++;
  }

  Serial.println("Key: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  if ( mfrc522hack.MIFARE_SetUid(uid, (byte)4, true) ) {
    Serial.println("Wrote new UID to card");
  }
}

