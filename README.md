# Arduino-Aime-Reader
使用 Arduino + PN532 + WS2812B 制作的 Aime 读卡器。支持 Felica 和 Aime（Mifare 卡模拟 Felica 是可选功能）。     
通信数据格式可参考 [card.txt](https://github.com/Sucareto/Arduino-Aime-Reader/blob/main/doc/card.txt) 和 [nfc.txt](https://github.com/Sucareto/Arduino-Aime-Reader/blob/main/doc/nfc.txt)。   
替换 chunihook.dll 可在控制台输出通信数据，源码在 [sg-cmd.c](https://github.com/Sucareto/Arduino-Chunithm-Reader/blob/main/tools/sg-cmd.c)。   

#### 使用方法：  
按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库；
Arduino 和 PN532 接好 VCC，GND，SDA，SCL；  
接上 WS2812B 灯条（可选）；  
上传程序。  

打开设备管理器，设置 Arduino 的 COM 号，一些参考如下：  
Chunithm：COM12，支持 Felica 和 Aime；  
Ongeki：COM1，仅支持 Aime；  
MaiMai Finale：COM2，仅支持 Aime；  

某些 Arduino 可能需要在打开主程序前给串口发送 DTR/RTS，需要先打开一次 Arduino 串口监视器再启动主程序。  

#### 引用库：  
[驱动WS2812B FastLED.h](https://github.com/FastLED/FastLED)    
[驱动PN532 PN532.h](https://github.com/elechouse/PN532)    
