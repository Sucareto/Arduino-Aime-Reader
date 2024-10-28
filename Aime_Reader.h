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

uint8_t KeyA[6], KeyB[6]; // 用于存储 MIFARE key

enum { // 命令标记
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
  CMD_TO_UPDATER_MODE = 0x60,
  CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  CMD_FIRMWARE_UPDATE = 0x64,
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

enum { // FeliCa 专用，在 CMD_FELICA_THROUGH 命令使用
  FelicaPolling = 0x00,
  FelicaReqResponce = 0x04,
  FelicaReadWithoutEncryptData = 0x06,
  FelicaWriteWithoutEncryptData = 0x08,
  FelicaReqSysCode = 0x0C,
  FelicaActive2 = 0xA4,
};

enum { // 命令执行状态，res 数据包专用
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

typedef union { // 大小为 128 bytes 的联合体，用于存储收到的请求命令数据
  uint8_t bytes[128];
  struct {
    uint8_t frame_len; // 数据包长度，不包含转义符
    uint8_t addr;
    uint8_t seq_no; // 数据包序号
    uint8_t cmd; // 命令标记
    uint8_t payload_len; // 后续数据长度
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

typedef union {// 大小为 128 bytes 的联合体，用于存储读卡器准备回复的数据
  uint8_t bytes[128];
  struct {
    uint8_t frame_len; // 数据包长度，不包含转义符
    uint8_t addr;
    uint8_t seq_no; // 数据包序号，需要和请求包对应
    uint8_t cmd;// 命令标记
    uint8_t status; // 命令执行状态标记
    uint8_t payload_len;// 后续数据长度
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
            uint8_t blockData[4][16];
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

packet_request_t req;
packet_response_t res;

// 读取数据状态，作为全局对象初始化，在 packet_read 读取超时后，下次执行可以接上进度
uint8_t len, r, checksum;
bool escape = false;

uint8_t packet_read() { // 数据包读取函数
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) { // 检测到包头，重置包长度
      req.frame_len = 0xFF;
      continue;
    }
    if (req.frame_len == 0xFF) { // 设置包长度
      req.frame_len = r;
      len = 0;
      checksum = r;
      continue;
    }
    if (r == 0xD0) { // 读取到转义符，设置转义标记
      escape = true;
      continue;
    }
    if (escape) { // 转义处理
      r++;
      escape = false;
    }
    req.bytes[++len] = r;
    if (len == req.frame_len) { // 长度正确且校验通过，则返回命令标记，否则返回 STATUS_SUM_ERROR
      if (req.cmd == CMD_FIRMWARE_UPDATE) return CMD_FIRMWARE_UPDATE; //如果命令为0x64，则不校验checksum
      return checksum == r ? req.cmd : STATUS_SUM_ERROR;
    }
    checksum += r; // 包头后每位数据（不含转义）相加，作为校验值
  }
  return 0; // 数据包未读取完成
}

void packet_write() {
  uint8_t checksum = 0, len = 0;
  if (res.cmd == 0) { // 无待发数据
    return;
  }
  SerialDevice.write(0xE0);
  while (len <= res.frame_len) {
    uint8_t w;
    if (len == res.frame_len) { // 包数据已写入完成，发送校验值
      w = checksum;
    } else {
      w = res.bytes[len];
      checksum += w; // 包头后每位数据（不含转义）相加，作为校验值
    }
    if (w == 0xE0 || w == 0xD0) { // 转义符写入
      SerialDevice.write(0xD0);
      SerialDevice.write(--w);
    } else {
      SerialDevice.write(w);
    }
    len++;
  }
  res.cmd = 0;
}

void res_init(uint8_t payload_len = 0) { // 通用回复生成，参数指定不含包头的数据长度
  res.frame_len = 6 + payload_len;
  res.addr = req.addr;
  res.seq_no = req.seq_no;
  res.cmd = req.cmd;
  res.status = STATUS_OK; // 默认命令执行状态标记
  res.payload_len = payload_len;
}

void sys_to_normal_mode() { // 作用未知，根据 cmd 猜测
  res_init();
  if (nfc.getFirmwareVersion()) {
    res.status = STATUS_INVALID_COMMAND; // 在 837-15396 和 TN32MSEC003S 串口数据确认
  } else {
    res.status = STATUS_INTERNAL_ERROR;
    FastLED.showColor(0xFF0000);
  }
}

void sys_get_fw_version() { // 版本数据，通过串口数据确认，在读卡器测试界面显示
  res_init(sizeof(fw_version) - 1);
  memcpy(res.version, fw_version, res.payload_len);
}

void sys_get_hw_version() { // 版本数据，通过串口数据确认，在读卡器测试界面显示
  res_init(sizeof(hw_version) - 1);
  memcpy(res.version, hw_version, res.payload_len);
}

void sys_get_led_info() { // 版本数据，通过串口数据确认
  res_init(sizeof(led_info) - 1);
  memcpy(res.version, led_info, res.payload_len);
}

void nfc_start_polling() { // 作用未知，根据 cmd 猜测，开始读卡
  res_init();
  nfc.setRFField(0x00, 0x01);
}

void nfc_stop_polling() { // 作用未知，根据 cmd 猜测，停止读卡
  res_init();
  nfc.setRFField(0x00, 0x00);
}

void nfc_card_detect() { // 读取卡片类型
  uint16_t SystemCode;
  uint8_t bufferLength;
  // MIFARE
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, res.mifare_uid, &res.id_len)) {
    res_init(res.id_len + 3);
    res.count = 1;
    res.type = 0x10;
  // FeliCa
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

void nfc_mifare_authorize_a() { // 对 MIFARE 使用 KeyA 认证
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 0, KeyA)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_authorize_b() {// 对 MIFARE 使用 KeyB 认证
  res_init();
  if (!nfc.mifareclassic_AuthenticateBlock(req.uid, 4, req.block_no, 1, KeyB)) {
    res.status = STATUS_CARD_ERROR;
  }
}

void nfc_mifare_read() { // 认证成功后，读取 MIFARE 指定的 block
  res_init(0x10);
  if (!nfc.mifareclassic_ReadDataBlock(req.block_no, res.block)) {
    res_init();
    res.status = STATUS_CARD_ERROR;
  }
}

// 游戏发送的0x71命令后面的包实际上是完整的与Felica卡片直接通信的包，可以转发进PN532库直接用
// response也可以直接打包转发回游戏
void nfc_felica_through() { // FeliCa 处理函数
  uint8_t response_length = 0;
  req.encap_code &= 0x0F; //把0xA4处理为0x04（FELICA_CMD_REQUEST_RESPONSE）
  if (nfc.inDataExchange(&req.encap_len, req.encap_len, &res.encap_len, response_length)) {
    res_init(response_length);
    //如果成功的话 res.encap_len == response_length
  } else {
    res_init(0);
    res.status = STATUS_CARD_ERROR;
    // 数据交换失败时返回STATUS_CARD_ERROR触发重传
    // 不对数据交换失败时做特殊处理，直接返回STATUS_CARD_ERROR。可能导致部分卡片（国产手机+aicemu）读取失败。
  }
}
