#include "Aime_Reader.h"

// 该定义存在时，使用 115200 波特率，型号为 837-15396

#define high_baudrate

// 该定义存在时，跳过实际的 FeliCa 操作
// 仅通过 CMD_CARD_DETECT 发送 IDm、PMm、SystemCode
#define SKIP_FeliCa_THROUGH



void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.showColor(0);
  nfc.begin();
  while (!nfc.getFirmwareVersion()) { // 检测不到 PN532 时，循环红灯闪烁，不再继续初始化
    FastLED.showColor(0xFF0000);
    delay(500);
    FastLED.showColor(0);
    delay(500);
  }
  nfc.setPassiveActivationRetries(0x10);
  nfc.SAMConfig();
  // 初始化数据包结构体对象
  memset(req.bytes, 0, sizeof(req.bytes));
  memset(res.bytes, 0, sizeof(res.bytes));

  SerialDevice.begin(baudrate);
  FastLED.showColor(BootColor);
}

void loop() {
  // 从串口读取数据，数据校验成功后，跳转到对应的函数执行操作
  switch (packet_read()) {
    case 0: // 数据包未读取完成
      break;

    case CMD_TO_NORMAL_MODE:
      sys_to_normal_mode();
      break;
    case CMD_GET_FW_VERSION:
      sys_get_fw_version();
      break;
    case CMD_GET_HW_VERSION:
      sys_get_hw_version();
      break;

    // 读卡和 PN532 操作
    case CMD_START_POLLING:
      nfc_start_polling();
      break;
    case CMD_STOP_POLLING:
      nfc_stop_polling();
      break;
    case CMD_CARD_DETECT:
      nfc_card_detect();
      break;

    // MIFARE 处理
    case CMD_MIFARE_KEY_SET_A:
      memcpy(KeyA, req.key, 6);
      res_init();
      break;

    case CMD_MIFARE_KEY_SET_B:
      res_init();
      memcpy(KeyB, req.key, 6);
      break;

    case CMD_MIFARE_AUTHORIZE_A:
      nfc_mifare_authorize_a();
      break;

    case CMD_MIFARE_AUTHORIZE_B:
      nfc_mifare_authorize_b();
      break;

    case CMD_MIFARE_READ:
      nfc_mifare_read();
      break;

#ifndef SKIP_FeliCa_THROUGH
    // FeliCa 读写
    case CMD_FELICA_THROUGH:
      nfc_felica_through();
      break;
#endif

    // LED
    case CMD_EXT_BOARD_LED_RGB: // 设置 LED 颜色，无需回复
      FastLED.showColor(CRGB(req.color_payload[0], req.color_payload[1], req.color_payload[2]));
      break;

    case CMD_EXT_BOARD_INFO:
      sys_get_led_info();
      break;

    case CMD_EXT_BOARD_LED_RGB_UNKNOWN: // 未知，仅在使用 emoney 时出现
      break;

    case CMD_CARD_SELECT:
    case CMD_CARD_HALT:
    case CMD_EXT_TO_NORMAL_MODE:
    case CMD_TO_UPDATER_MODE: // 作用未知，根据串口数据猜测
      res_init();
      break;

    case CMD_SEND_HEX_DATA: // 非 TN32MSEC003S 时，可能会触发固件更新逻辑
      res_init();
      res.status = STATUS_COMP_DUMMY_3RD;
      // 回复 STATUS_COMP_DUMMY_3RD 可以跳过更新
      // 如果因 fw、hw 变动导致触发更新，该回复会导致更新失败，amdaemon 崩溃
      break;

    case STATUS_SUM_ERROR: // 读取数据校验失败时的回复，未确认效果
      res_init();
      res.status = STATUS_SUM_ERROR;
      break;

    default: // 对于其他未知的数据默认处理方式，未确认效果
      res_init();
      res.status = STATUS_INVALID_COMMAND;
  }
  packet_write();
}
