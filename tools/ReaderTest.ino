#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

uint16_t systemCode = 0xFFFF;
uint8_t requestCode = 0x01;
uint16_t systemCodeResponse;
typedef union {
  uint8_t block[16];
  struct {
    uint8_t IDm[8];
    uint8_t PMm[8];
  };
} Card_Data;

Card_Data card;
uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
uint8_t BanaKey[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define M2F_B 1

void setup() {
  SerialUSB.begin(38400);
  SerialUSB.setTimeout(0);
  while (!SerialUSB);
  nfc.begin();
  while (!nfc.getFirmwareVersion()) {
    SerialUSB.println("Didn't find PN53x board");
    delay(2000);
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
}

void loop() {
  uint8_t uid[4], uL;
  delay(1000);

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 1, AimeKey)) {
    SerialUSB.println("Aime");
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, BanaKey)) {
    SerialUSB.println("Bana");
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, M2F_B, 0, MifareKey)) {
    SerialUSB.println("Default Key Mifare");
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL)) {
    SerialUSB.println("Mifare:Unknown key");
    return;
  }

  if (nfc.felica_Polling(systemCode, requestCode, card.IDm, card.PMm, &systemCodeResponse, 200)) {
    SerialUSB.println("Found a Felica card!");
    return;
  }
  SerialUSB.println("Didn't find card");
}
