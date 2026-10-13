// PAGE_ENV 渲染实现（坐标/字号/颜色见 ui_layout.h 与布局稿 v0.4 §3）
#include "ui/page_env.h"

#include "display/font_data.h"
#include "ui/comfort.h"
#include "ui/ui_layout.h"

namespace {

void drawSmall(FrameBuffer& fb, char ch, int16_t x, int16_t y, Color c) {
  fb.drawChar(ch, x, y, c, FontKind::SMALL);
}

}  // namespace

namespace page_env {

void render(FrameBuffer& fb, int16_t tempTenths, uint8_t humPercent,
            bool tempValid, bool humValid) {
  using ui_layout::kEnvY;

  // ---- 温度组（左）：十位 / 个位 / 小数点 / 十分位 / °C ----
  if (tempValid) {
    const Color c = comfortColorTemp(tempTenths);
    int16_t v = tempTenths;
    const bool neg = v < 0;
    if (neg) {
      v = static_cast<int16_t>(-v);
    }
    if (v > 999) {
      v = 999;  // 显示上限 99.9
    }
    const uint8_t t10 = static_cast<uint8_t>(v / 100);
    const uint8_t t1 = static_cast<uint8_t>((v / 10) % 10);
    const uint8_t t1th = static_cast<uint8_t>(v % 10);
    if (neg) {
      drawSmall(fb, '-', ui_layout::kEnvT10X, kEnvY, c);  // 负号占十位槽
    } else if (t10 != 0) {
      drawSmall(fb, static_cast<char>('0' + t10), ui_layout::kEnvT10X, kEnvY, c);
    }
    drawSmall(fb, static_cast<char>('0' + t1), ui_layout::kEnvT1X, kEnvY, c);
    fb.setPixel(ui_layout::kEnvDecX, ui_layout::kEnvDecY, c);  // 小数点（间隙列）
    drawSmall(fb, static_cast<char>('0' + t1th), ui_layout::kEnvT1thX, kEnvY, c);
    drawSmall(fb, static_cast<char>(kCodeDegC), ui_layout::kEnvDegCX, kEnvY, c);
  } else {
    drawSmall(fb, '-', ui_layout::kEnvT10X, kEnvY, Color::RED);
    drawSmall(fb, '-', ui_layout::kEnvT1X, kEnvY, Color::RED);
  }

  // ---- 湿度组（右）：十位 / 个位 / % ----
  if (humValid) {
    const Color c = comfortColorHum(humPercent);
    const uint8_t h10 = humPercent / 10;
    const uint8_t h1 = humPercent % 10;
    if (h10 != 0) {
      drawSmall(fb, static_cast<char>('0' + h10), ui_layout::kEnvH10X, kEnvY, c);
    }
    drawSmall(fb, static_cast<char>('0' + h1), ui_layout::kEnvH1X, kEnvY, c);
    drawSmall(fb, '%', ui_layout::kEnvPctX, kEnvY, c);
  } else {
    drawSmall(fb, '-', ui_layout::kEnvH10X, kEnvY, Color::RED);
    drawSmall(fb, '-', ui_layout::kEnvH1X, kEnvY, Color::RED);
  }
}

}  // namespace page_env
