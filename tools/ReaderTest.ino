#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

//#define SerialDevice SerialUSB //32u4,samd21
#define SerialDevice Serial //esp8266

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

uint16_t systemCode = 0xFFFF;
uint8_t requestCode = 0x01;
typedef union {
  uint8_t block[18];
  struct {
    uint8_t IDm[8];
    uint8_t PMm[8];
    union {
      uint16_t SystemCode;
      uint8_t System_Code[2];
    };
  };
} Card;
Card card;

uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
uint8_t BanaKey[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define M2F_B 1

void setup() {
  SerialDevice.begin(38400);
  SerialDevice.setTimeout(0);
  while (!SerialDevice);
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    SerialDevice.println("Didn't find PN53x board");
    delay(2000);
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
}

void loop() {
  uint8_t uid[4], uL;
  delay(2000);

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 1, AimeKey)) {
    SerialDevice.print("Aime card!UID Value:");
    nfc.PrintHex(uid, uL);
    SerialDevice.print("Block 2 Data:");
    if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
      nfc.PrintHex(card.block, 16);
    }
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, BanaKey)) {
    SerialDevice.println("Banapassport card!UID Value:");
    nfc.PrintHex(uid, uL);
    SerialDevice.print("Block 2 Data:");
    if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
      nfc.PrintHex(card.block, 16);
    }
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, M2F_B, 0, MifareKey)) {
    SerialDevice.println("Default Key Mifare!");
    if (nfc.mifareclassic_ReadDataBlock(2, card.block)) {
      SerialDevice.print(" Fake IDm:");
      nfc.PrintHex(card.IDm, 8);
      SerialDevice.print("Fake PMm:");
      nfc.PrintHex(card.PMm, 8);
    }
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL)) {
    SerialDevice.println("Unknown key Mifare.UID Value:");
    nfc.PrintHex(uid, uL);
    return;
  }

  if (nfc.felica_Polling(systemCode, requestCode, card.IDm, card.PMm, &card.SystemCode, 200)) {
    SerialDevice.println("Felica card!");
    SerialDevice.print("IDm:");
    nfc.PrintHex(card.IDm, 8);
    SerialDevice.print("PMm:");
    nfc.PrintHex(card.PMm, 8);
    SerialDevice.print("SystemCode:");
    nfc.PrintHex(card.System_Code, 2);
    return;
  }
  SerialDevice.println("Didn't find card");
}
