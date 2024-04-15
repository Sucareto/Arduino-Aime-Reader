#if defined(__AVR_ATmega32U4__)
#pragma message "当前的开发板是 ATmega32U4"
#define SerialDevice SerialUSB
#define PN532_SPI_SS 10
#define LED_PIN A3

#elif defined(ESP8266)
#pragma message "当前的开发板是 ESP8266"
#define SerialDevice Serial
#define PN532_SPI_SS D4
#define LED_PIN D5

#elif defined(ESP32)
#pragma message "当前的开发板是 ESP32"
#define SerialDevice Serial
#define PN532_SPI_SS 5
#define LED_PIN 13

#else
#error "未经测试的开发板，请检查串口和针脚定义"
#endif

#if defined(PN532_SPI_SS)
#pragma message "使用 SPI 连接 PN532"
#include <PN532_SPI.h>
PN532_SPI pn532(SPI, PN532_SPI_SS);

#elif defined(PN532_HSU_Device)
#pragma message "使用 HSU 连接 PN532"
#include <PN532_HSU.h>
PN532_HSU pn532(PN532_HSU_Device);

#else
#pragma message "使用 I2C 连接 PN532"
#include <PN532_I2C.h>
PN532_I2C pn532(Wire);
#endif

#include "PN532.h"
PN532 nfc(pn532);

#ifdef high_baudrate
#pragma message "high_baudrate 已启用"
#define baudrate 115200
#define BootColor 0x0000FF
#define fw_version "\x94"
#define hw_version "837-15396"
#define led_info "000-00000\xFF\x11\x40"

#else
#define baudrate 38400
#define BootColor 0x00FF00
#define fw_version "TN32MSEC003S F/W Ver1.2"
#define hw_version "TN32MSEC003S H/W Ver3.0"
#define led_info "15084\xFF\x10\x00\x12"
#endif

#include "FastLED.h"
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

uint8_t KeyA[6], KeyB[6];

enum {
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING = 0x41,
  CMD_CARD_DETECT = 0x42,
  CMD_CARD_SELECT = 0x43,
  CMD_CARD_HALT = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ = 0x52,
  // CMD_MIFARE_WRITE = 0x53,
  CMD_MIFARE_KEY_SET_B = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  // CMD_TO_UPDATER_MODE = 0x60,
  // CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  // CMD_SEND_BINDATA_INIT = 0x63,
  // CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  // CMD_FELICA_PUSH = 0x70,
  CMD_FELICA_THROUGH = 0x71,
  // LED board
  CMD_EXT_BOARD_LED = 0x80,
  CMD_EXT_BOARD_LED_RGB = 0x81,
  CMD_EXT_BOARD_LED_RGB_UNKNOWN = 0x82,  // 未知
  CMD_EXT_BOARD_INFO = 0xf0,
  CMD_EXT_FIRM_SUM = 0xf2,
  CMD_EXT_SEND_HEX_DATA = 0xf3,
  CMD_EXT_TO_BOOT_MODE = 0xf4,
  CMD_EXT_TO_NORMAL_MODE = 0xf5,
};

enum {
  FelicaPolling = 0x00,
  FelicaReqResponce = 0x04,
  FelicaReadWithoutEncryptData = 0x06,
  FelicaWriteWithoutEncryptData = 0x08,
  FelicaReqSysCode = 0x0C,
  FelicaActive2 = 0xA4,
};

enum {
  STATUS_OK = 0x00,
  STATUS_CARD_ERROR = 0x01,
  STATUS_NOT_ACCEPT = 0x02,
  STATUS_INVALID_COMMAND = 0x03,
  STATUS_INVALID_DATA = 0x04,
  STATUS_SUM_ERROR = 0x05,
  STATUS_INTERNAL_ERROR = 0x06,
  STATUS_INVALID_FIRM_DATA = 0x07,
  STATUS_FIRM_UPDATE_SUCCESS = 0x08,
  STATUS_COMP_DUMMY_2ND = 0x10,
  STATUS_COMP_DUMMY_3RD = 0x20,
};

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t key[6];            // CMD_MIFARE_KEY_SET
      uint8_t color_payload[3];  // CMD_EXT_BOARD_LED_RGB
      struct {                   // CMD_CARD_SELECT,AUTHORIZE,READ
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {  // CMD_FELICA_THROUGH_POLL
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct {  // CMD_FELICA_THROUGH_READ,WRITE,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            uint8_t blockList[1][2];  // CMD_FELICA_THROUGH_READ
            uint8_t blockData[16];    // CMD_FELICA_THROUGH_WRITE
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_request_t;

typedef union {
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      uint8_t version[1];  // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
      uint8_t block[16];   // CMD_MIFARE_READ
      struct {             // CMD_CARD_DETECT
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t mifare_uid[7];  // 可以读取 MIFARE Ultralight，但游戏不支持
          struct {
            uint8_t IDm[8];
            uint8_t PMm[8];
          };
        };
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {  // FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];
            uint8_t numBlock;
            uint8_t blockData[1][1][16];
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

packet_request_t req;
packet_response_t res;

uint8_t len, r, checksum;
bool escape = false;

uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) {
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len) {
      return checksum == r ? req.cmd : STATUS_SUM_ERROR;
    }
    checksum += r;
  }
  return 0;
}

void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) {
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) {
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w;
    }
    if (w == 0xE0 || w == 0xD0) {
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void res_init(uint8_t payload_len = 0) {
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = STATUS_OK;
  res.payload_len = payload_len;
}

void sys_to_normal_mode() {
  res_init();
  if (nfc.getFirmwareVersion()) {
    res.status = STATUS_INVALID_COMMAND;
  } else {
    res.status = STATUS_INTERNAL_ERROR;
    FastLED.showColor(0xFF0000);
  }
}

void sys_get_fw_version() {
  res_init(sizeof(fw_version) - 1);
  memcpy(res.version, fw_version, res.payload_len);
}

void sys_get_hw_version() {
  res_init(sizeof(hw_version) - 1);
  memcpy(res.version, hw_version, res.payload_len);
}

void sys_get_led_info() {
  res_init(sizeof(led_info) - 1);
  memcpy(res.version, led_info, res.payload_len);
}

void nfc_start_polling() {
  res_init();
  nfc.setRFField(0x00, 0x01);
}

void nfc_stop_polling() {
  res_init();
  nfc.setRFField(0x00, 0x00);
}

void nfc_card_detect() {
  uint16_t SystemCode;
  uint8_t bufferLength;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)) {
    res_init(res.id_len + 3);
    res.count = 1;
    res.type = 0x10;
  } else if (nfc.felica_Polling(0xFFFF, 0x00, res.IDm, res.PMm, &SystemCode, 200) == 1) {
    res_init(0x13);
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
  } else {
    res_init(1);
    res.count = 0;
  }
}

void nfc_mifare_authorize_a() {
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 0, KeyA)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_authorize_b() {
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 1, KeyB)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_read() {
  res_init(0x10);
  if (!nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    res_init();
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_felica_through() {
  uint16_t SystemCode;
  if (nfc.felica_Polling(0xFFFF, 0x01, res.encap_IDm, res.poll_PMm, &SystemCode, 200) == 1) {
    SystemCode = SystemCode >> 8 | SystemCode << 8;
  } else {
    res_init();
    res.status = STATUS_CARD_ERROR;
    return;
  }
  uint8_t code = req.encap_code;
  res.encap_code = code + 1;
  switch (code) {
    case FelicaPolling:
      {
        res_init(0x14);
        res.poll_systemCode[0] = SystemCode;
        res.poll_systemCode[1] = SystemCode >> 8;
      }
      break;
    case FelicaReqSysCode:
      {
        res_init(0x0D);
        res.felica_payload[0] = 0x01;
        res.felica_payload[1] = SystemCode;
        res.felica_payload[2] = SystemCode >> 8;
      }
      break;
    case FelicaActive2:
      {
        res_init(0x0B);
        res.felica_payload[0] = 0x00;
      }
      break;
    case FelicaReadWithoutEncryptData:
      {
        uint16_t serviceCodeList[1] = { (uint16_t)(req.serviceCodeList[1] << 8 | req.serviceCodeList[0]) };
        for (uint8_t i = 0; i < req.numBlock; i++) {
          uint16_t blockList[1] = { (uint16_t)(req.blockList[i][0] << 8 | req.blockList[i][1]) };
          if (nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, res.blockData[i]) != 1) {
            memset(res.blockData[i], 0, 16);  // dummy data
          }
        }
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
        res.numBlock = req.numBlock;
        res_init(0x0D + req.numBlock * 16);
      }
      break;
    case FelicaWriteWithoutEncryptData:
      {
        res_init(0x0C);  // WriteWithoutEncryption,ignore
        res.RW_status[0] = 0;
        res.RW_status[1] = 0;
      }
      break;
    default:
      res_init();
      res.status = STATUS_INVALID_COMMAND;
  }
  res.encap_len = res.payload_len;
}
