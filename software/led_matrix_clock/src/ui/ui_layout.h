#pragma once
// UI 像素布局常量（照布局稿 docs/ui-layout.md v0.4 定稿）
// 改版只动本头 + 布局稿，渲染代码引用常量不动
#include <stdint.h>

namespace ui_layout {

// ---- TIME 页（布局稿 §2）：31 列主线，大 4x7 y=0..6、小 3x5 y=2..6 ----
constexpr int16_t kBigY = 0;      // 大字顶行
constexpr int16_t kSmallY = 2;    // 小字顶行（与大字底对齐 y=6）
constexpr int16_t kH1X = 0;       // 时十位 x=0..3
constexpr int16_t kH2X = 5;       // 时个位 x=5..8
constexpr int16_t kColonX = 10;   // 冒号点列（y=2 与 y=4）
constexpr int16_t kColonY1 = 2;
constexpr int16_t kColonY2 = 4;
constexpr int16_t kM1X = 12;      // 分十位 x=12..15
constexpr int16_t kM2X = 17;      // 分个位 x=17..20
constexpr int16_t kS1X = 24;      // 秒十位 x=24..26
constexpr int16_t kS2X = 28;      // 秒个位 x=28..30

// 12h 标识（定稿=乙）：右上角 3x2 色块，AM=GREEN / PM=ORANGE
constexpr int16_t kAmPmX = 28;
constexpr int16_t kAmPmY = 0;
constexpr int16_t kAmPmW = 3;
constexpr int16_t kAmPmH = 2;

// ---- ENV 页（布局稿 §3）：全 3x5，7 槽位 27 列 ----
constexpr int16_t kEnvY = 2;
constexpr int16_t kEnvT10X = 0;    // 温度十位
constexpr int16_t kEnvT1X = 4;     // 温度个位
constexpr int16_t kEnvDecX = 7;    // 小数点（间隙列，底行 y=6）
constexpr int16_t kEnvDecY = 6;
constexpr int16_t kEnvT1thX = 8;   // 温度十分位
constexpr int16_t kEnvDegCX = 12;  // °C
constexpr int16_t kEnvH10X = 16;   // 湿度十位
constexpr int16_t kEnvH1X = 20;    // 湿度个位
constexpr int16_t kEnvPctX = 24;   // %

}  // namespace ui_layout
