# LED 矩阵时钟（ESP8266）

ESP8266 + 4 块共阳双色 8×8 点阵（TM1640×4）的桌面时钟：大字时间 + 温湿度轮显，
DS1302 掉电走时，WiFi/NTP 自动对时，光敏自动亮度，按键与网页配置。

实现框架：**PlatformIO + Arduino**（espressif8266），全新实现；
开源项目 [`MatrixClockV1.2.ino`](MatrixClockV1.2.ino) 仅作**功能参考**（不移植代码）。

## 功能

- **显示**：32×8 双色画布；时间页（大 4×7 时分 + 小 3×5 秒 + 冒号 1Hz 闪烁）与
  温湿度页（`25.3°C 45%`，按舒适度红/绿/橙着色）每 15s 整页轮换；数字垂直滚动、
  日期 30min 水平滚动；12h 模式右上角 AM(绿)/PM(橙) 色块；上电自检
- **时钟**：DS1302 + CR2032 掉电走时；SNTP 开机对时 + 每 6h 后台校时写 RTC；12/24h 可配
- **温湿度**：DHT11/DHT22 单总线、自动判别型号，3s 采样，失败保持上次值
- **网络**：WiFi 自动连接 + 断线重连；WiFiManager SoftAP 配网（长按 2s / 无凭据自动进入，
  180s 超时）；状态提示 `Wi-Fi` / `OK!!` / `Err!` / `Setup` / `RTC!`
- **交互与配置**：GPIO0 按键（短按切页 / 长按 2s 配网 / 长按 5s 恢复出厂）；
  5516 光敏自动亮度（8 档、迟滞防闪）；13 项配置持久化 flash，**网页可改**
- **工程化**：显示后端可换芯片（MAX7219 等单色后端忽略颜色，见需求 R-DRV-07）；
  54 项主机侧单测；独立显示验收固件（全字形/颜色/方向/亮度巡回）

## 硬件

| 部分 | 器件 | 说明 |
|---|---|---|
| 主控 | ESP8266（ESP-12E/F） | 2.4G WiFi |
| 显示 | TM1640 ×4 + 74HCT595 + 74HCT08 | 4 块共阳双色 8×8 点阵；595 出 DIN/片选，与门 gate SCLK（3.3→5V 电平） |
| 时钟 | DS1302 + 32.768kHz + CR2032 | 三线接口，3.3V 供电（I/O 不容忍 5V） |
| 传感器 | DHT11（兼容 DHT22） | 单总线 |
| 光敏 | 5516 + 分压 → A0 | 自动亮度 |
| 按键 | GPIO0（与 BOOT 复用） | 低有效 |

引脚分配、电源域、驱动时序与片选协议见 [硬件设计](docs/hardware-design.md)。

## 快速开始

```bash
cd software/led_matrix_clock

pio test -e native          # 主机侧单测（54 例，无硬件）
pio run -e esp12e           # 编译产品固件
pio run -e esp12e -t upload # 烧录（裸 ESP-12 需按住 GPIO0 上电进下载模式）
pio device monitor -b 115200

pio run -e esp12e_fonttest -t upload   # 显示验收固件（硬件联调用）
```

首次上电无 WiFi 凭据会自动进配网：连接热点 `MatrixClock-Setup`，浏览器打开
`192.168.4.1` 填 WiFi；联网后串口打印 Web 配置页地址（`[web] config page: http://<ip>/`）。

## 使用

| 操作 | 效果 |
|---|---|
| 短按 GPIO0 | 切换 时间 / 温湿度 页（重置 15s 轮换计时） |
| 长按 2s | 进入 SoftAP 配网（屏幕提示 `Setup`） |
| 长按 5s | 恢复出厂（清配置并重启） |
| 浏览器访问设备 IP | 网页配置 13 项（时区、NTP 服务器、12/24h、日期顺序、方向、冒号闪烁、亮度、默认页、轮换/采样/滚动周期），保存即校验入 flash |

配置项清单与生效时机（实时/重启）见 [需求 §8](docs/requirements.md)。

## 仓库结构

```
├── docs/                        设计文档
│   ├── hardware-design.md       硬件设计（电路、引脚、时序协议）
│   ├── requirements.md          需求基线 v1.4（功能清单、配置项、验收）
│   ├── design.md                架构/模块接口/测试与阶段计划
│   └── ui-layout.md             UI 像素布局稿（坐标/字号/颜色/示例帧）
├── hardware/datasheet/          器件数据手册（TM1640 等）
├── software/led_matrix_clock/   PlatformIO 固件工程
│   ├── src/
│   │   ├── app/                 协作式调度器
│   │   ├── config/              配置结构 + JSON 校验 + LittleFS 持久化
│   │   ├── display/             帧缓冲/字库/DisplayDevice 后端/glcdfont
│   │   ├── net/                 WiFiManager 配网 + SNTP + Web 配置
│   │   ├── rtc/                 DS1302 驱动 + 时间纯逻辑
│   │   ├── sensor/              DHT11/DHT22 驱动 + 解码/型号判别
│   │   ├── ui/                  页面渲染/动画/按键/亮度/覆盖层/布局常量
│   │   ├── apps/                显示验收固件入口（esp12e_fonttest）
│   │   └── main.cpp             产品固件入口
│   ├── test/                    Unity 单测（10 组）
│   └── tools/gen_font.py        七段字模生成器（4×7 / 3×5）
├── MatrixClockV1.2.ino          功能参考（仅参考，未移植）
└── LICENSE                      GPL-3.0
```

## 开发

- 架构分层、模块接口、时序/内存预算：[设计文档](docs/design.md)
- **显示解耦**：渲染层只依赖 `DisplayDevice` 接口与设备无关行位图；换驱动芯片
  （如 MAX7219，单色后端忽略颜色平面）只需新增一个后端实现
- **字模**：`tools/gen_font.py` 生成 `font_data.cpp`（勿手改）；英文/日期用 glcdfont（BSD-3）
- **测试**：纯逻辑（渲染转置/方向/双色、BCD、DHT 解码与判别、舒适度边界、动画合成、
  配置校验、按键/亮度状态机）全部主机侧单测；硬件项联调清单见设计文档 §13
- **提交规范**：英文 summary + description；代码注释中文
- 当前状态：P1–P8 全部实施，单测与双固件编译通过；**硬件统一联调待进行**

## 文档

| 文档 | 内容 |
|---|---|
| [硬件设计](docs/hardware-design.md) | 引脚分配、电源域、TM1640/74HCT595/74HCT08 电路、驱动时序与片选协议 |
| [需求文档](docs/requirements.md) | 参考代码功能提取、需求清单（定稿）、实现约束、配置项清单 |
| [设计文档](docs/design.md) | 架构分层、调度模型、模块接口、TM1640 指令映射、测试与阶段计划 |
| [UI 布局稿](docs/ui-layout.md) | 32×8 像素布局：七段样式数字（4×7/3×5）+ glcdfont、坐标/颜色、动画规则、示例帧 |

## 许可与致谢

- 本仓库代码：GPL-3.0（见 [LICENSE](LICENSE)）
- glcdfont 字库：Adafruit GFX（BSD-3，版权头保留于 `software/led_matrix_clock/src/display/glcdfont.c`）
- 功能参考：[MatrixClockV1.2.ino（立创开源广场）](https://oshwhub.com/Flight666/ESP8266-ju-zhen-shi-zhong)——
  仅参考其功能行为，未复制代码与字模
