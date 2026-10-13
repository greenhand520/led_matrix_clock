// 显示门面实现：委托 DisplayDevice 后端
#include "display/display.h"

#include <Arduino.h>

void Display::begin(uint8_t brightness) {
  dev_.begin(brightness);
}

void Display::flush(const FrameBuffer& fb) {
  dev_.writeFrame(fb);
}

void Display::setBrightness(uint8_t level) {
  dev_.setBrightness(level);
}

void Display::selfTest(FrameBuffer& fb) {
  fb.clear(Color::RED);
  flush(fb);
  delay(250);  // 启动期固定延时，仅此处允许
  fb.clear(Color::GREEN);
  flush(fb);
  delay(250);
  fb.clear();
  flush(fb);
}
