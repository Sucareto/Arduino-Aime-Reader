// 本程序已于 2023/07/09 停止开发
// 展示信息的小屏幕，波特率 115200，只会出现在支持 電子決済 的框体，设备仅接收数据，不需要回复
// 在启用 emoney 时，显示支持的卡片种类和付款情况
// com 序号在 config_common.json > emoney > display_port 定义
// https://github.com/djhackersdev/segatools/blob/master/board/vfd.c
// vfd 串口数据没有固定格式，没有长度位
// 测试程序显示文字用的是 u8g2 库，默认用的是 utf8 编码，无法直接使用 vfd 串口的 shift-jis 编码数据

// 字库相关的信息：
// https://github.com/olikraus/u8g2/discussions/1508
// https://github.com/larryli/u8g2_wqy
// https://github.com/breakstring/u8g2_fontmaker
// https://github.com/createskyblue/Easy-u8g2-font-generate-tools/blob/master/main.py

#define SerialDevice Serial

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

static uint8_t state = 0;
static char textBuffer[];
static uint8_t text_len = 0;
// https://r12a.github.io/app-encodings/
// https://www.mgo-tec.com/blog-entry-utf8-sjis-wroom-arduino-lib.html
// https://github.com/mgo-tec/UTF8SJIS_for_ESP8266/blob/master/src/UTF8toSJIS.cpp
// https://github.com/mgo-tec/ESP32_SD_UTF8toSJIS

void setup() {
  SerialDevice.begin(115200);
  SerialDevice.setTimeout(0);
  Wire.setClock(800000);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_mf);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void loop() {
  while (SerialDevice.available()) {
    uint8_t r = SerialDevice.read();
    if (r == 0x1B) {
      r = SerialDevice.read();
      state = 0;
      continue;
    }
    if (!state) {
      state = r;
      continue;
    }
    switch (state) {
      case 0x0B:  // 仅开机出现过
        // 1b 0b
        break;
      case 0x0C:
        // 1b 0c
        text_len = 0;
        break;
      case 0x21:  // 仅开机出现过，在 0b 后面
        // 1b 21 01
        break;
      case 0x30:
        textBuffer[text_len++] = r;
        continue;
        // 1b 30 00 00 00  93 64 8e 71 8c 88 8d cf 82 c5 82 ab 82 dc 82 b7        // 電子決済できます
        // 1b 30 00 00 00  93 64 8e 71 8c 88 8d cf 82 c5 82 ab 82 dc 82 b9 82 f1  // 電子決済できません
        // 1b 30 00 00 02  6e 61 6e 61 63 6f 8e 78 95 a5 20 5c 35 30 30           // anaco支払 \500
      case 0x32:  // 仅开机出现过，在 21 后面
        // 1b 32 02
        break;
      case 0x40:
        // 1b 40 00 00 (00|02) 00 9f 02
        uint8_t prm1 = SerialDevice.read();
        uint8_t prm2 = SerialDevice.read();
        uint8_t Line = SerialDevice.read();
        uint8_t BoxSize_x = SerialDevice.read();
        uint8_t BoxSize_y = SerialDevice.read();
        // 缺少一位
        break;
      case 0x41:  // 在 40 后面，speed 只发现是 00，用途未知
        // 1b 41 00
        uint8_t speed = SerialDevice.read();
        break;
      case 0x50:  // 长度后面是字符内容
        uint8_t len = SerialDevice.read();
        textBuffer[text_len++] = r;
        continue;
      case 0x51:  // 仅在有输出时，作为最后一条指令发送
        // 1b 51
        break;
      case 0x52:  // 跟在 0c 后面
        // 1b 52
        break;
    }
  }
}

/*
// oled 显示测试
#include <Wire.h>
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
// https://github.com/olikraus/u8g2/wiki/fntlistallplain
// https://github.com/olikraus/u8g2/wiki/u8g2reference

char text1[] = {
  0xe9, 0x9b, 0xbb,
  0xe5, 0xad, 0x90,
  0xe6, 0xb1, 0xba,
  0xe6, 0xb8, 0x88,
  0xe3, 0x81, 0xa7,
  0xe3, 0x81, 0x8d,
  0xe3, 0x81, 0xbe,
  0xe3, 0x81, 0x99,
  0
};

char text2[] = {
  0x6e, 0x61, 0x6e, 0x61, 0x63, 0x6f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe4, 0xba, 0xa4, 0xe9, 0x80, 0x9a, 0xe7, 0xb3, 0xbb, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x57, 0x41, 0x4f, 0x4e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x50, 0x41, 0x53, 0x45, 0x4c, 0x49, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0
};


u8g2_uint_t titleW, msgW, DisplayW;    // 字符、屏幕宽度
u8g2_uint_t title_offset, msg_offset;  // 字符的偏移位置

bool is_display = false;
uint8_t state, title_len, msg_len;
char title_buffer[256];
char msg_buffer[256];

void setup() {
  SerialDevice.begin(115200);
  SerialDevice.setTimeout(0);
  Wire.setClock(800000);
  u8g2.begin();
  u8g2.setFont(u8g2_font_unifont_t_japanese3);
  u8g2.enableUTF8Print();
  Serial.begin(115200);


  DisplayW = u8g2.getDisplayWidth();
  // Serial.println(titleW);
  // Serial.println(msgW);
  // Serial.println(DisplayW);
}


void displayTask() {
  if (!is_display) {
    return;
  }
  u8g2.clearBuffer();
  if (titleW) {
    // SerialDevice.println("titleW");
    if (titleW < DisplayW) {
      u8g2.drawUTF8(0, 14, title_buffer);
      // u8g2.setCursor(0, 14);
      // u8g2.print(text1);
    } else {
      u8g2_uint_t x = title_offset;
      do {
        u8g2.drawUTF8(x, 14, title_buffer);
        x += titleW;
      } while (x < DisplayW);
      title_offset -= 1;
      if ((u8g2_uint_t)title_offset < (u8g2_uint_t)-titleW) {
        title_offset = 0;
      }
    }
  }

  if (msgW) {
    // SerialDevice.println("msgW");
    if (msgW < DisplayW) {
      u8g2.drawUTF8(0, 30, msg_buffer);
    } else {
      u8g2_uint_t x = msg_offset;
      do {
        u8g2.drawUTF8(x, 30, msg_buffer);
        x += msgW;
      } while (x < DisplayW);
      msg_offset -= 1;
      if ((u8g2_uint_t)msg_offset < (u8g2_uint_t)-msgW) {
        msg_offset = 0;
      }
    }
  }
  u8g2.sendBuffer();
  // delay(500);
}



void loop() {
  msg_len = 0;
  // memset(title_buffer, 0, 256);
  // memset(msg_buffer, 0, 256);
  // bool pre_display = false;
  while (SerialDevice.available()) {
    msg_buffer[msg_len++] = SerialDevice.read();
    // uint8_t r = SerialDevice.read();
    // if (r == 0x1B) {
    //   state = 0;
    //   continue;
    // }
    // if (!state) {
    //   state = SerialDevice.read();
    // }
    // switch (state) {
    //   case 0x0C:
    //     title_len = 0;
    //     msg_len = 0;
    //     titleW = 0;
    //     msgW = 0;
    //     is_display = false;
    //     // memset(title_buffer, 0, 256);
    //     // memset(msg_buffer, 0, 256);
    //     u8g2.clearBuffer();
    //     u8g2.sendBuffer();
    //     SerialDevice.println("clear");
    //     break;
    //   case 0x30:
    //     title_buffer[title_len++] = r;
    //     pre_display = true;
    //     continue;
    //   case 0x50:
    //     msg_buffer[msg_len++] = r;
    //     pre_display = true;
    //     continue;
    //   default:
    //     break;
    // }
  }
  // if (pre_display) {
  // if (title_len) {
  //   title_buffer[title_len++] = 0;
  //   titleW = u8g2.getUTF8Width(title_buffer);
  //   title_offset = 0;
  //   is_display = true;
  //   SerialDevice.println("pre_display title_len");
  // }
  if (msg_len) {
    msg_buffer[msg_len++] = 0;
    msgW = u8g2.getUTF8Width(msg_buffer);
    SerialDevice.println(msgW);
    msg_offset = 0;
    is_display = true;
    SerialDevice.println("pre_display msg_len");
  }
  // pre_display = false;
  // }


  displayTask();
}

*/