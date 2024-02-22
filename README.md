# Arduino-Aime-Reader
使用 Arduino + PN532 制作的 Aime 兼容读卡器。   
**当前更新内容未经过长时间测试，如果遇到问题可以回滚到[稳定的 v1.0 版本](https://github.com/Sucareto/Arduino-Aime-Reader/tree/v1.0)**   
English: [lawliuwuu/Arduino-Aime-Reader](https://github.com/lawliuwuu/Arduino-Aime-Reader)   

- 支持卡片类型： [FeliCa](https://zh.wikipedia.org/wiki/FeliCa)（Amusement IC、Suica、八达通等）和 [MIFARE](https://zh.wikipedia.org/wiki/MIFARE)（Aime，Banapassport）
- 逻辑实现是通过对官方读卡器串口数据进行分析猜测出来的，并非逆向，不保证正确实现
- 通信数据格式参考了 [Segatools](https://github.com/djhackersdev/segatools) 和官方读卡器抓包数据，可在 [Example.txt](doc/Example.txt) 和 [nfc.txt](https://github.com/djhackersdev/segatools/blob/master/doc/nfc.txt) 查看
- 定制使用例（主要的开发测试环境）：[ESP32-CardReader](https://github.com/Sucareto/ESP32-CardReader) 


### 使用方法：
1. 按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库
2. 按照使用方式，在 Arduino 和 PN532 接好连接线（I2C 或 SPI），并调整 PN532 上的拨码开关
3. 接上 WS2812B 灯条（可选，不会影响正常读卡功能）
4. 上传 [ReaderTest](tools/ReaderTest/ReaderTest.ino) 测试硬件是否工作正常
5. 若读卡正常，可按照支持列表打开设备管理器设置 COM 端口号
6. 按照游戏的波特率设置代码的`high_baudrate`选项，`115200`是`true`，`38400`是`false`
7. 如果有使用 [Segatools](https://github.com/djhackersdev/segatools)，参考 [segatools.ini 设置教程](https://github.com/djhackersdev/segatools/blob/master/doc/config/common.md#enable) 关闭 Aime 模拟读卡器
8. 上传程序打开游戏测试

如果需要自定义 Aime 卡，安装 [MifareClassicTool](https://github.com/ikarus23/MifareClassicTool)，修改 [Aime 卡示例](doc/aime示例.mct) 后写入空白 MIFARE UID/CUID 卡，即可刷卡使用。  
关于自定义 Aime 卡的写入和读取问题，请参考 [SAK（88->08）](https://github.com/Sucareto/Arduino-Aime-Reader/pull/17)的讨论。  
某些 Arduino 可能需要在游戏主程序连接前给串口以正确的波特率发送 DTR/RTS，需要先打开一次 Arduino 串口监视器再启动主程序。  


### 支持游戏列表：
| 代号 | 默认 COM 号 | 支持的卡 | 默认波特率 |
| - | - | - | - |
| SDDT/SDEZ | COM1 | FeliCa,MIFARE | 115200 |
| SDEY | COM2 | MIFARE | 38400 |
| SDHD | COM4 | FeliCa,MIFARE | cvt=38400,sp=115200 |
| SBZV/SDDF | COM10 | FeliCa,MIFARE | 38400 |
| SDBT | COM12 | FeliCa,MIFARE | 38400 |

- 如果读卡器没有正常工作，可以切换波特率试下
- 有使用 amdaemon 的，可以参考 config_common.json 内 aime > unit > port 确认端口号
- 如果 `"high_baudrate" : true` 则波特率是`115200`，否则就是`38400`
- 在 `"high_baudrate" : true` 的情况下，本读卡器程序支持 emoney 功能，端末认证和刷卡支付均正常（需要游戏和服务器支持）


### 开发板适配情况：
| 开发板名 | 主控 | 备注 |
| - | - | - |
| SparkFun Pro Micro | ATmega32U4 | 需要发送 DTR/RTS，未完全测试 |
| SparkFun SAMD21 Dev Breakout | ATSAMD21G18 | 未完全测试 |
| NodeMCU 1.0 | ESP-12E + CP2102 & CH340 | SDA=D2，SCL=D1 |
| NodeMCU-32S | ESP32-S + CH340 | 主要适配环境 |
| Arduino Uno | ATmega328P + CH340 | 未实际测试，据反馈不可用 |


### 已知问题：
- 在 NDA_08 命令的写入 Felica 操作没有实现，因为未确认是否会影响卡片后续使用
- 未确定`res.status`错误码的定义，因此`res.status`的值可能是错误的
- 因为 PN532 库不支持同时读取多张卡片，所以未实现`mifare_select_tag`，只会读到最先识别的卡片


### 引用库：  
- 驱动 WS2812B：[FastLED](https://github.com/FastLED/FastLED)
- 驱动 PN532：[PN532](https://github.com/elechouse/PN532) 或 [Aime_Reader_PN532](https://github.com/Sucareto/Aime_Reader_PN532)
- 读取 FeliCa 参考：[PN532を使ってArduinoでFeliCa学生証を読む方法](https://qiita.com/gpioblink/items/91597a5275862f7ffb3c)
- 读取 FeliCa 数据的程序：[NFC TagInfo](https://play.google.com/store/apps/details?id=at.mroland.android.apps.nfctaginfo)，[NFC TagInfo by NXP](https://play.google.com/store/apps/details?id=com.nxp.taginfolite)
