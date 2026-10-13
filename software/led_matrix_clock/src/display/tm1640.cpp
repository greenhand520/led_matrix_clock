// TM1640 链驱动实现（GPIO 位带，全部时序为微秒级短脉冲）
#include "display/tm1640.h"

#include <Arduino.h>

namespace {
constexpr uint8_t kPinSer = 13;    // 595 SER（硬件设计引脚表）
constexpr uint8_t kPinSrclk = 14;  // 595 SRCLK
constexpr uint8_t kPinRclk = 12;   // 595 RCLK
constexpr uint8_t kPinSclk = 15;   // TM1640 共享 SCLK（经与门）
constexpr uint8_t kDinBit = 4;     // 帧 D4 = DIN
constexpr uint8_t kChipCount = 4;

constexpr int kHalf595Us = 1;  // 595 移位半周期
constexpr int kHalfTmUs = 2;   // TM1640 半周期（覆盖 PWCLK/tWAIT）

// 显示控制：0x90|lv = 开+亮度 lv（脉宽 1/16,2/16,4/16,10/16..14/16）；0x80 = 关
constexpr uint8_t kCmdDataAuto = 0x40;   // 数据命令：地址自动加 1
constexpr uint8_t kCmdAddrBase = 0xC0;   // 地址命令：C0H + n
constexpr uint8_t kCmdOnBase = 0x90;
constexpr uint8_t kCmdOff = 0x80;
}  // namespace

void Tm1640Chain::shift595(uint8_t frame) {
  for (int i = 7; i >= 0; --i) {  // MSB 先行（D7 最先移入，落 Q7）
    digitalWrite(kPinSer, (frame >> i) & 1);
    digitalWrite(kPinSrclk, HIGH);
    delayMicroseconds(kHalf595Us);
    digitalWrite(kPinSrclk, LOW);
    delayMicroseconds(kHalf595Us);
  }
  digitalWrite(kPinRclk, HIGH);  // 锁存：DIN/SEL 变化发生在这一刻
  delayMicroseconds(kHalf595Us);
  digitalWrite(kPinRclk, LOW);
  delayMicroseconds(kHalf595Us);
}

void Tm1640Chain::apply() {
  shift595(static_cast<uint8_t>(sel_ | (din_ ? (1 << kDinBit) : 0)));
}

void Tm1640Chain::setDin(bool level) {
  din_ = level;
  apply();
}

void Tm1640Chain::selectChip(uint8_t chip) {
  sel_ = static_cast<uint8_t>(1 << chip);
  apply();
}

void Tm1640Chain::deselectAll() {
  sel_ = 0;
  din_ = false;
  apply();
}

void Tm1640Chain::sclkHigh() {
  digitalWrite(kPinSclk, HIGH);
  delayMicroseconds(kHalfTmUs);
}

void Tm1640Chain::sclkLow() {
  digitalWrite(kPinSclk, LOW);
  delayMicroseconds(kHalfTmUs);
}

void Tm1640Chain::clkPulse() {
  sclkHigh();
  sclkLow();
}

void Tm1640Chain::start() {
  // Start = SCLK 高时 DIN 下降沿（手册 P4）；此跳变必须在 SCLK 高电平锁存
  setDin(true);
  sclkHigh();
  setDin(false);
  sclkLow();
}

void Tm1640Chain::stop() {
  // End = SCLK 高时 DIN 上升沿；随后回空闲（SCLK 低、DIN 低）
  sclkHigh();
  setDin(true);
  sclkLow();
  setDin(false);
}

void Tm1640Chain::writeByte(uint8_t b) {
  for (uint8_t i = 0; i < 8; ++i) {
    setDin((b >> i) & 1);  // 数据位仅在 SCLK 低电平变化；LSB 先行
    clkPulse();
  }
}

void Tm1640Chain::begin() {
  pinMode(kPinSer, OUTPUT);
  pinMode(kPinSrclk, OUTPUT);
  pinMode(kPinRclk, OUTPUT);
  pinMode(kPinSclk, OUTPUT);
  digitalWrite(kPinSer, LOW);
  digitalWrite(kPinSrclk, LOW);
  digitalWrite(kPinRclk, LOW);
  digitalWrite(kPinSclk, LOW);  // 空闲低（GPIO15 上电须低）
  din_ = false;
  sel_ = 0;
  apply();

  // 手册建议：上电显存清零，否则开屏可能出乱码
  const uint8_t zeros[16] = {0};
  for (uint8_t m = 0; m < kChipCount; ++m) {
    writeRam(m, zeros);
  }
  setBrightness(level_);
  setDisplayOn(true);
}

void Tm1640Chain::writeRam(uint8_t chip, const uint8_t ram[16]) {
  selectChip(chip);
  start();
  writeByte(kCmdDataAuto);
  stop();
  start();
  writeByte(kCmdAddrBase);
  for (uint8_t i = 0; i < 16; ++i) {
    writeByte(ram[i]);
  }
  stop();
  deselectAll();
}

void Tm1640Chain::setBrightness(uint8_t level) {
  level_ = (level > 7) ? 7 : level;
  for (uint8_t m = 0; m < kChipCount; ++m) {
    selectChip(m);
    start();
    writeByte(static_cast<uint8_t>(kCmdOnBase | level_));
    stop();
  }
  deselectAll();
}

void Tm1640Chain::setDisplayOn(bool on) {
  for (uint8_t m = 0; m < kChipCount; ++m) {
    selectChip(m);
    start();
    writeByte(on ? static_cast<uint8_t>(kCmdOnBase | level_) : kCmdOff);
    stop();
  }
  deselectAll();
}
