# Arduino-Aime-Reader
使用 Arduino + PN532 + WS2812B 制作的 Aime 兼容读卡器。    
支持 [FeliCa](https://zh.wikipedia.org/wiki/FeliCa)（Amusement IC、Suica、八达通等）和 [MIFARE](https://zh.wikipedia.org/wiki/MIFARE)（Aime，Banapassport）。    
实现逻辑为官方读卡器串口数据对比 + 脑补，不保证正确实现。    
通信数据格式参考了 [Segatools]() 和官方读卡器抓包数据，可在 [Example.txt](doc/Example.txt) 和 [nfc.txt](doc/nfc.txt) 查看。   


### 使用方法：
1. 按照 [PN532](https://github.com/elechouse/PN532) 的提示安装库
2. Arduino 和 PN532 接好 VCC，GND，SDA，SCL
3. PN532 的拨码开关按照 PCB 上丝印的指示，调整到 I2C 模式
4. 接上 WS2812B 灯条（可选）
5. 上传 [ReaderTest](tools/ReaderTest/ReaderTest.ino) 测试硬件是否工作正常
6. 若读卡正常，可按照支持列表打开设备管理器设置 COM 端口号
7. 按照游戏的波特率设置代码的`high_baudrate`选项
8. 上传程序打开游戏测试
9. 安装 [MifareClassicTool](https://github.com/ikarus23/MifareClassicTool)，修改 [Aime 卡示例](doc/aime示例.mct) 后写入空白 MIFARE UID/CUID 卡

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


### 已知问题：
- Felica 在非 amdaemon 游戏可能无法正常工作，因为 NDA_06 未正确回复
- 未确定`res.status`的意义，因此`res.status = 1;`可能是错误的
- 因为`get_fw`和`get_hw`返回的是自定义版本号，启动时可能触发 amdaemon 的固件升级，可以将 aime_firm 文件夹重命名或删除
- 未实现`mifare_select_tag`，未支持多卡选择，只会读到最先识别的卡片


### 引用库：  
- [驱动WS2812B FastLED.h](https://github.com/FastLED/FastLED)
- [驱动PN532 PN532.h](https://github.com/elechouse/PN532)
