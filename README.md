# Arduino-Chunithm-Reader
使用 Arduino + PN532 + WS2812B 制作的 Aime 读卡器。    
通信数据格式可参考 [card.txt](https://github.com/Sucareto/Arduino-Aime-Reader/blob/main/doc/card.txt) 和 [nfc.txt](https://github.com/Sucareto/Arduino-Aime-Reader/blob/main/doc/nfc.txt)。   
替换 chunihook.dll 可在控制台输出通信数据，源码在 [sg-cmd.c](https://github.com/Sucareto/Arduino-Chunithm-Reader/blob/main/tools/sg-cmd.c)。   
#### 引用库：  
[驱动WS2812B FastLED.h](https://github.com/FastLED/FastLED)    
[驱动PN532 Adafruit_PN532.h](https://github.com/adafruit/Adafruit-PN532)    
