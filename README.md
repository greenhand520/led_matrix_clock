# LED 矩阵时钟（ESP8266）

ESP8266 + 4 块共阳双色 8×8 点阵（TM1640×4）的桌面时钟：DS1302 掉电走时、
DHT11 温湿度、WiFi/NTP 自动对时、GPIO0 按键交互。

实现框架：**PlatformIO + Arduino**（espressif8266）

## 文档

| 文档 | 内容 |
|---|---|
| [硬件设计](docs/hardware-design.md) | 引脚分配、电源域、TM1640/74HCT595/74HCT08 电路、驱动时序与片选协议 |
| [需求文档 v1.1](docs/requirements.md) | 参考代码功能提取、需求清单（定稿）、实现约束、配置项清单 |
| [设计文档 v0.1](docs/design.md) | 架构分层、调度模型、模块接口、TM1640 指令映射、测试与阶段计划（待确认） |

## 仓库结构

```
├── docs/
│   ├── hardware-design.md      硬件设计（定稿电路方案）
│   └── requirements.md         需求文档（v1.1 定稿基线）
├── hardware/
│   └── datasheet/              器件数据手册
└── README.md
```
