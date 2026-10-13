// 显示门面实现
#include "display/display.h"

#include <Arduino.h>

void Display::begin(uint8_t brightness) {
  chain_.begin();
  chain_.setBrightness(brightness);
}

void Display::flush(const FrameBuffer& fb) {
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  for (uint8_t m = 0; m < FrameBuffer::kModules; ++m) {
    chain_.writeRam(m, ram[m]);
  }
}

void Display::setBrightness(uint8_t level) {
  chain_.setBrightness(level);
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
