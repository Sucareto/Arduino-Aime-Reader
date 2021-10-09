#include "FastLED.h"
#define NUM_LEDS 6
#define DATA_PIN D5//14
CRGB leds[NUM_LEDS];

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

uint8_t cardtype, uid[4], uL;
uint16_t systemCode = 0xFFFF;
uint8_t requestCode = 0x01;
typedef union {
  uint8_t block[18];
  struct {
    uint8_t IDm[8];
    uint8_t PMm[8];
    uint16_t SystemCode;
  };
} Felica;
Felica felica;
uint8_t AimeKey[6], BanaKey[6];

#ifdef M2F
#define M2F_B 1 //指定从mifare读取第几block当作felica的IDm和PMm
uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif

enum {
  SG_NFC_CMD_GET_FW_VERSION       = 0x30,//获取 FW 版本
  SG_NFC_CMD_GET_HW_VERSION       = 0x32,//获取 HW 版本
  SG_RGB_CMD_GET_INFO             = 0xF0,//获取 LED 信息

  SG_NFC_CMD_RESET                = 0x62,//重置读卡器
  SG_RGB_CMD_SET_COLOR            = 0x81,//LED 颜色设置
  SG_RGB_CMD_RESET                = 0xF5,//LED 重置
  SG_NFC_CMD_RADIO_ON             = 0x40,//打开读卡器
  SG_NFC_CMD_RADIO_OFF            = 0x41,//关闭读卡器
  SG_NFC_CMD_POLL                 = 0x42,//发送卡号

  SG_NFC_CMD_MIFARE_SELECT_TAG    = 0x43,
  SG_NFC_CMD_MIFARE_SET_KEY_BANA  = 0x50,
  SG_NFC_CMD_MIFARE_SET_KEY_AIME  = 0x54,
  SG_NFC_CMD_MIFARE_READ_BLOCK    = 0x52,
  SG_NFC_CMD_MIFARE_AUTHENTICATE  = 0x55,
  SG_NFC_CMD_FELICA_ENCAP         = 0x71,

  //FELICA_ENCAP
  FELICA_CMD_POLL                 = 0x00,
  FELICA_CMD_GET_SYSTEM_CODE      = 0x0C,
  FELICA_CMD_NDA_A4               = 0xA4,
  FELICA_CMD_NDA_06               = 0x06,//测试中，作用未知
  FELICA_CMD_NDA_08               = 0x08,//测试中，作用未知
};

typedef union packet_req {
  uint8_t bytes[256];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t payload_len;
    union {
      uint8_t key[6];// sg_nfc_req_mifare_set_key(bana or aime)
      uint8_t color_payload[3];//sg_led_req_set_color
      struct {// sg_nfc_cmd_mifare_read_block
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {// sg_nfc_req_felica_encap
        uint8_t IDm[8];
        uint8_t encap_len;
        uint8_t code;
        uint8_t felica_payload[241];
      };
    };
  };

} packet_req_t;

typedef union packet_res {
  uint8_t bytes[256];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      char version[23];// sg_nfc_res_get_fw_version,sg_nfc_res_get_hw_version
      uint8_t reset_payload;  //sg_led_res_reset
      uint8_t info_payload[9]; //sg_led_res_get_info
      uint8_t block[16];// sg_nfc_res_mifare_read_block
      struct {// sg_nfc_res_poll
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        uint8_t IDm[8];//and Aime uid
        uint8_t PMm[8];
      };
      struct {// sg_nfc_res_felica_encap
        uint8_t encap_len;
        uint8_t code;
        uint8_t encap_IDm[8];
        union {
          struct {//FELICA_CMD_POLL
            uint8_t encap_PMm[8];
            uint8_t system_code[2];
          };
          struct {//NDA06
            uint8_t NDA06_code[3];
            uint8_t NDA06_IDm[8];
            uint8_t NDA06_Data[8];
          };
          uint8_t felica_payload[241];
        };
      };
    };
  };
} packet_res_t;

static packet_req_t req;
static packet_res_t res;

static void sg_res_init(uint8_t payload_len = 0) { //初始化模板
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = 0;
  res.payload_len = payload_len;
}

static void sg_nfc_cmd_reset() {//重置读卡器
  nfc.begin();
  nfc.setPassiveActivationRetries(0x10);//设定等待次数
  nfc.SAMConfig();
  if (nfc.getFirmwareVersion()) {
    nfc.SAMConfig();
    sg_res_init();
    res.status = 3;
    return;
  }
  fill_solid(leds, NUM_LEDS, 0xFFFF00);
  FastLED.show();
}

static void sg_nfc_cmd_get_fw_version() {
  sg_res_init(23);
  //  memcpy(res.version, "TN32MSEC003S F/W Ver1.2", 23);
  memcpy(res.version, "Sucareto Aime Reader FW", 23);
}

static void sg_nfc_cmd_get_hw_version() {
  sg_res_init(23);
  //  memcpy(res.version, "TN32MSEC003S H/W Ver3.0", 23);
  memcpy(res.version, "Sucareto Aime Reader HW", 23);
}

static void sg_nfc_cmd_mifare_set_key() {
  sg_res_init();
  if (req.cmd == SG_NFC_CMD_MIFARE_SET_KEY_BANA) {
    memcpy(BanaKey, req.key, 6);
  } else if (req.cmd == SG_NFC_CMD_MIFARE_SET_KEY_AIME) {
    memcpy(AimeKey, req.key, 6);
  }
}

static void sg_led_cmd_reset() {
  sg_res_init();
  FastLED.clear();
  FastLED.show();
}

static void sg_led_cmd_get_info() {
  sg_res_init(9);
  static uint8_t info[9] = {'1', '5', '0', '8', '4', 0xFF, 0x10, 0x00, 0x12};
  memcpy(res.info_payload, info, 9);
}

static void sg_led_cmd_set_color() {
  uint8_t r = req.color_payload[0];
  uint8_t g = req.color_payload[1];
  uint8_t b = req.color_payload[2];
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show();
}

static void sg_nfc_cmd_radio_on() {
  cardtype = 0;
  uL = 0;
  memset(&uid, 0, 4);
  memset(&felica, 0, 16);
  sg_res_init();
  //uid,uidLength,block,keyA(0) or keyB(1),Key
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 1, AimeKey)) {
    cardtype = 0x10;
    return;
  }
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, 1, 0, BanaKey)) {
    cardtype = 0x10;
    return;
  }
  if (nfc.felica_Polling(systemCode, requestCode, felica.IDm, felica.PMm, &felica.SystemCode, 200)) {
    cardtype = 0x20;
    return;
  }
#ifdef M2F
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uL) && nfc.mifareclassic_AuthenticateBlock(uid, uL, M2F_B, 0, MifareKey)) {
    cardtype = 0x30;
    return;
  }
#endif
}

static void sg_nfc_cmd_radio_off() {
  sg_res_init();
}

static void sg_nfc_cmd_poll() { //卡号发送
  if (cardtype == 0x10) {
    res.count = 1;
    res.type = 0x10;
    res.id_len = uL;
    memcpy(res.IDm, uid, uL);
    sg_res_init(7);
    return;
  }
  if (cardtype == 0x20) {
    sg_res_init(19);
    memcpy(res.IDm, felica.IDm, 8);
    memcpy(res.PMm, felica.PMm, 8);
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
    return;
  }
#ifdef M2F
  if (cardtype == 0x30) {
    sg_res_init(19);
    nfc.mifareclassic_ReadDataBlock(M2F_B, felica.block);
    memcpy(res.IDm, felica.IDm, 8);
    memcpy(res.PMm, felica.PMm, 8);
    felica.block[16] = 0x00;
    felica.block[17] = 0x03;//suica
    res.count = 1;
    res.type = 0x20;
    res.id_len = 0x10;
    return;
  }
#endif
  sg_res_init(1);
  res.count = 0;
}

static void sg_nfc_cmd_mifare_select_tag() {
  sg_res_init();
}

static void sg_nfc_cmd_mifare_authenticate() {
  sg_res_init();
}

static void sg_nfc_cmd_mifare_read_block() {//读取卡扇区数据
  sg_res_init();
  if (req.block_no > 2) {
    return;
  }
  if (nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    sg_res_init(0x10);
    return;
  }
}

static void sg_nfc_cmd_felica_encap() {
  uint8_t code = req.code;
  res.code = code + 1;
  switch (code) {
    case FELICA_CMD_POLL:
      sg_res_init(0x14);
      memcpy(res.encap_IDm, felica.IDm, 8);
      memcpy(res.encap_PMm, felica.PMm, 8);
      memcpy(res.system_code, &felica.SystemCode, 2);
      break;
    case FELICA_CMD_GET_SYSTEM_CODE:
      sg_res_init(0x0D);
      memcpy(res.IDm, felica.IDm, 8);
      res.felica_payload[0] = 0x01;//未知
      res.felica_payload[1] = felica.block[16];//SystemCode
      res.felica_payload[2] = felica.block[17];
      break;
    case FELICA_CMD_NDA_A4:
      sg_res_init(0x0B);
      res.felica_payload[0] = 0x00;
      break;
    case FELICA_CMD_NDA_06:
      sg_res_init(0x1D);
      memcpy(res.encap_IDm, felica.IDm, 8);
      res.NDA06_code[0] = 0x00;
      res.NDA06_code[1] = 0x00;
      res.NDA06_code[2] = 0x01;//未知
      memcpy(res.NDA06_IDm, felica.IDm, 8);
      memcpy(res.NDA06_Data, felica.PMm, 8);//填补数据用
      break;
    case FELICA_CMD_NDA_08:
      sg_res_init(0x0C);
      memcpy(res.encap_IDm, felica.IDm, 8);
      res.felica_payload[0] = 0x00;
      res.felica_payload[1] = 0x00;
      break;
    default:
      sg_res_init();
      res.status = 1;
  }
  res.encap_len = res.payload_len;
}
