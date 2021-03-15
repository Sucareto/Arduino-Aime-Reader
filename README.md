# Arduino-Chunithm-Reader
使用 Arduino + PN532 + WS2812B 制作的 Chunithm 读卡器。 
使用 38400 波特率监听 COM12 端口和 aimeReaderHost 通信，通信数据格式可参考 card.txt 和 nfc.txt。
替换 chunihook.dll 可在控制台输出通信数据，源码在 sg-cmd.c。

#### 引用库：  
[驱动WS2812B FastLED.h](https://github.com/FastLED/FastLED)
[驱动PN532 Adafruit_PN532.h](https://github.com/adafruit/Adafruit-PN532)
