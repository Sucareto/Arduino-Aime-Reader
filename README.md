# Arduino-Aime-Reader
使用 Arduino + PN532 + WS2812B 制作的 Aime 兼容读卡器。   
**目前所有主要功能已经实现，如果没有 bug 应该不会再更新。**   
English: [lawliuwuu/Arduino-Aime-Reader](https://github.com/lawliuwuu/Arduino-Aime-Reader)   

支持卡片类型： [FeliCa](https://zh.wikipedia.org/wiki/FeliCa)（Amusement IC、Suica、八达通等）和 [MIFARE](https://zh.wikipedia.org/wiki/MIFARE)（Aime，Banapassport）。   
实现逻辑为官方读卡器串口数据对比 + 脑补，不保证正确实现。    
通信数据格式参考了 [Segatools]() 和官方读卡器抓包数据，可在 [Example.txt](doc/Example.txt) 和 [nfc.txt](doc/nfc.txt) 查看。   
一个使用例：[ESP32-CardReader](https://github.com/Sucareto/ESP32-CardReader)   

### 使用方法：
1. 按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库
2. 按照使用方式，在 Arduino 和 PN532 接好连接线（I2C或SPI），并调整 PN532 上的拨码开关
3. 接上 WS2812B 灯条（可选）
4. 上传 [ReaderTest](tools/ReaderTest/ReaderTest.ino) 测试硬件是否工作正常
5. 若读卡正常，可按照支持列表打开设备管理器设置 COM 端口号
6. 按照游戏的波特率设置代码的`high_baudrate`选项，`115200`是`true`，`38400`是`false`
7. 上传程序打开游戏测试
8. 安装 [MifareClassicTool](https://github.com/ikarus23/MifareClassicTool)，修改 [Aime 卡示例](doc/aime示例.mct) 后写入空白 MIFARE UID/CUID 卡

某些 Arduino 可能需要在游戏主程序连接前给串口以正确的波特率发送 DTR/RTS，需要先打开一次 Arduino 串口监视器再启动主程序。  
如果是 SDBT，可以在启动前运行一次 [DTR-RTS.exe](tools/DTR-RTS.exe) 以向 COM1 和 COM12 发送DTR/RTS。  
如果需要向其他端口和特定的波特率发送，可以修改 [DTR-RTS.c](tools/DTR-RTS.c) 然后编译。


### 支持列表：
| 游戏代号 | COM端口号 | 支持的卡 | 默认波特率 |
| - | - | - | - |
| SDDT/SDEZ | COM1 | FeliCa,MIFARE | 115200 |
| SDEY | COM2 | MIFARE | 38400 |
| SDHD | COM4 | FeliCa,MIFARE | cvt=38400,sp=115200 |
| SBZV/SDDF | COM10 | FeliCa,MIFARE | 38400 |
| SDBT | COM12 | FeliCa,MIFARE | 38400 |

- 如果读卡器没有正常工作，可以切换波特率试下
- 有使用 amdaemon 的，可以参考 config_common.json 内 aime > unit > port 确认端口号
- 如果 `"high_baudrate" : true` 则波特率是`115200`，否则就是`38400`


### 已测试开发板：
- SparkFun Pro Micro（ATmega32U4），需要发送 DTR/RTS
- SparkFun SAMD21 Dev Breakout（ATSAMD21G18）
- NodeMCU 1.0（ESP-12E + CP2102 & CH340），SDA=D2，SCL=D1
- NodeMCU-32S（ESP32-S + CH340）


### 已知问题：
- 在 NDA_08 命令的写入 Felica 操作没有实现，因为未确认是否会影响卡片后续使用
- 未确定`res.status`的意义，因此`res.status = 1;`可能是错误的
- 未实现`mifare_select_tag`，未支持多卡选择，只会读到最先识别的卡片


### 引用库：  
- 驱动 WS2812B：[FastLED](https://github.com/FastLED/FastLED)
- 驱动 PN532：[PN532](https://github.com/elechouse/PN532)
- 读取 FeliCa 参考：[PN532を使ってArduinoでFeliCa学生証を読む方法](https://qiita.com/gpioblink/items/91597a5275862f7ffb3c)
- 读取 FeliCa 数据的程序：[NFC TagInfo](https://play.google.com/store/apps/details?id=at.mroland.android.apps.nfctaginfo)，[NFC TagInfo by NXP](https://play.google.com/store/apps/details?id=com.nxp.taginfolite)
