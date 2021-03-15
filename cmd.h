#include "FastLED.h"
#define NUM_LEDS 6
#define DATA_PIN A3
#define BRI 50
CRGB leds[NUM_LEDS];

#include "Adafruit_PN532.h"
#define PN532_IRQ   (4)
Adafruit_PN532 nfc(PN532_IRQ, 16);
boolean readerDisabled = false;

uint8_t AimeKey[6];
uint8_t MifareKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

enum {
  SG_NFC_CMD_GET_FW_VERSION = 0x30,//获取FW版本
  SG_NFC_CMD_GET_HW_VERSION = 0x32,//获取HW版本
  SG_NFC_CMD_RADIO_ON = 0x40,//打开读卡器天线
  SG_NFC_CMD_RADIO_OFF = 0x41,//关闭读卡器天线
  SG_NFC_CMD_POLL = 0x42,//发送卡号
  SG_NFC_CMD_MIFARE_SELECT_TAG = 0x43,
  SG_NFC_CMD_MIFARE_SET_KEY_BANA = 0x50,
  SG_NFC_CMD_MIFARE_READ_BLOCK = 0x52,
  SG_NFC_CMD_MIFARE_SET_KEY_AIME = 0x54,
  SG_NFC_CMD_MIFARE_AUTHENTICATE = 0x55,
  SG_NFC_CMD_RESET = 0x62,//重置读卡器
  SG_NFC_CMD_FELICA_ENCAP = 0x71,
  SG_RGB_CMD_SET_COLOR = 0x81,//LED颜色设置
  SG_RGB_CMD_RESET = 0xF5,//LED重置
  SG_RGB_CMD_GET_INFO = 0xF0,//LED信息获取
  FELICA_CMD_POLL             = 0x00,//ENCAP用
  FELICA_CMD_GET_SYSTEM_CODE  = 0x0c,
  FELICA_CMD_NDA_A4           = 0xa4,
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
      uint8_t key[6];// sg_nfc_req_mifare_set_key
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
      uint8_t color_payload[3];//sg_led_req_set_color
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
      struct {// sg_nfc_res_poll
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t uid[4];
          uint8_t IDPM[16];
        };
      };
      uint8_t block[16];// sg_nfc_res_mifare_read_block
      struct {// sg_nfc_res_felica_encap
        uint8_t encap_len;
        uint8_t code;
        uint8_t IDm[8];
        union {
          struct {
            uint8_t PMm[8];
            uint8_t system_code[2];
          };
          uint8_t felica_payload[241];
        };
      };
      uint8_t reset_payload;  //sg_led_res_reset
      uint8_t info_payload[9]; //sg_led_res_get_info
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
  sg_res_init();
  res.status = 3;
  readerDisabled = true;
}

static void sg_nfc_cmd_get_fw_version() {
  sg_res_init(sizeof(res.version));
  memcpy(res.version, "TN32MSEC003S F/W Ver1.2E", sizeof(res.version));
}

static void sg_nfc_cmd_get_hw_version() {
  sg_res_init(sizeof(res.version));
  memcpy(res.version, "TN32MSEC003S H/W Ver3.0J", sizeof(res.version));
}

static void sg_nfc_cmd_mifare_set_key_aime() {
  sg_res_init();
  memcpy(req.key, AimeKey, sizeof(AimeKey));
}

static void sg_led_cmd_reset() {
  sg_res_init(sizeof(res.reset_payload));
  res.reset_payload = 0;
  fill_solid(leds, NUM_LEDS, 0x000000);
  FastLED[0].show(leds, NUM_LEDS, BRI);
}

static void sg_led_cmd_get_info() {
  sg_res_init(sizeof(res.info_payload));
  static uint8_t info[] = {'1', '5', '0', '8', '4', 0xFF, 0x10, 0x00, 0x12};
  memcpy(res.info_payload, info, 9);
}

static void sg_led_cmd_set_color() {
  uint8_t r = req.color_payload[0];
  uint8_t g = req.color_payload[1];
  uint8_t b = req.color_payload[2];
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED[0].show(leds, NUM_LEDS, BRI);
}

static void sg_nfc_cmd_radio_on() {
  sg_res_init();
  if (!readerDisabled) {
    return;
  }
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  readerDisabled = false;
}

static void sg_nfc_cmd_radio_off() {
  sg_res_init();
  readerDisabled = true;
}

static void sg_nfc_cmd_poll() { //卡号发送
  uint8_t uid[4];
  uint8_t t;
  //读aime（实验性）
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  delay(10);
  if (!digitalRead(PN532_IRQ)) {
    nfc.readDetectedPassiveTargetID(uid, &t);
    if (nfc.mifareclassic_AuthenticateBlock(uid, t, 1, MIFARE_CMD_AUTH_B, AimeKey)) { //aime
      res.type = 0x10;
      res.id_len = t;
      memcpy(res.uid, uid, t);//默认ID为0x01020304
      sg_res_init(7);
      res.count = 1;
      return;
    }
  }
  //读普通mifare当作felica
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  delay(10);
  if (!digitalRead(PN532_IRQ)) {
    nfc.readDetectedPassiveTargetID(uid, &t);
    if (nfc.mifareclassic_AuthenticateBlock(uid, t, 1, MIFARE_CMD_AUTH_A, MifareKey)) {
      if (nfc.mifareclassic_ReadDataBlock(1, res.IDPM)) {//此处略过了IDm和PMm的分别读取
        res.type = 0x20;
        res.id_len = sizeof(res.IDPM);
        sg_res_init(19);//count,type,id_len,IDm,PMm
        res.count = 1;
        return;
      }
    }
  }
  sg_res_init(1);
  res.count = 0;
}

static void sg_nfc_cmd_mifare_read_block() {
  if (req.block_no > 3)
    return;
  sg_res_init(sizeof(res.block));
  if (nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, MIFARE_CMD_AUTH_B, AimeKey)) {
    if (nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
      return;
    }
  }
  sg_res_init();
}

static void sg_nfc_cmd_felica_encap() {
  uint8_t code = req.code;
  res.code = code + 1;
  memcpy(res.IDm, req.IDm, sizeof(req.IDm));
  switch (code) {
    case FELICA_CMD_POLL:
      sg_res_init(0x14);
      res.system_code[0] = 0x00;
      res.system_code[1] = 0x00;
      break;
    case FELICA_CMD_GET_SYSTEM_CODE:
      sg_res_init(0x0D);
      res.felica_payload[0] = 0x01;
      res.felica_payload[1] = 0x00;
      res.felica_payload[2] = 0x00;
      break;
    case FELICA_CMD_NDA_A4:
      sg_res_init(0x0B);
      res.felica_payload[0] = 0x00;
      break;
  }
  res.encap_len = res.payload_len;
}
