// PAGE_TIME 渲染实现（坐标/字号/颜色见 ui_layout.h 与布局稿 v0.4 §2）
#include "ui/page_time.h"

#include "ui/ui_layout.h"

namespace page_time {

void render(FrameBuffer& fb, const DateTime& t, const Config& cfg) {
  using ui_layout::kBigY;
  using ui_layout::kSmallY;

  // 小时（12h 模式转 1..12，前导 0 不点亮）
  bool pm = false;
  uint8_t h = t.hour;
  if (cfg.hour_format == 12) {
    h = to12h(t.hour, &pm);
  }
  const uint8_t h1 = h / 10;
  const uint8_t h2 = h % 10;
  if (h1 != 0 || cfg.hour_format != 12) {
    fb.drawChar(static_cast<char>('0' + h1), ui_layout::kH1X, kBigY, Color::RED, FontKind::BIG);
  }
  fb.drawChar(static_cast<char>('0' + h2), ui_layout::kH2X, kBigY, Color::RED, FontKind::BIG);

  // 分
  fb.drawChar(static_cast<char>('0' + t.min / 10), ui_layout::kM1X, kBigY, Color::RED, FontKind::BIG);
  fb.drawChar(static_cast<char>('0' + t.min % 10), ui_layout::kM2X, kBigY, Color::RED, FontKind::BIG);

  // 冒号：colon_blink=0 常亮，否则 1Hz（秒奇偶）
  if (cfg.colon_blink == 0 || (t.sec % 2) == 0) {
    fb.setPixel(ui_layout::kColonX, ui_layout::kColonY1, Color::RED);
    fb.setPixel(ui_layout::kColonX, ui_layout::kColonY2, Color::RED);
  }

  // 秒（小 3x5，绿）
  fb.drawChar(static_cast<char>('0' + t.sec / 10), ui_layout::kS1X, kSmallY, Color::GREEN, FontKind::SMALL);
  fb.drawChar(static_cast<char>('0' + t.sec % 10), ui_layout::kS2X, kSmallY, Color::GREEN, FontKind::SMALL);

  // 12h 标识（定稿=乙）：右上角 3x2 色块，AM=GREEN / PM=ORANGE
  if (cfg.hour_format == 12) {
    const Color chip = pm ? Color::ORANGE : Color::GREEN;
    for (int16_t r = 0; r < ui_layout::kAmPmH; ++r) {
      for (int16_t c = 0; c < ui_layout::kAmPmW; ++c) {
        fb.setPixel(ui_layout::kAmPmX + c, ui_layout::kAmPmY + r, chip);
      }
    }
  }
}

}  // namespace page_time
