#pragma once
// TM1640 x4 + 74HCT595 + 74HCT08 驱动（GPIO 位带）
// 帧协议与时序见硬件设计「74HCT595 输出帧」「驱动时序与片选协议」
//
// 595 帧字节：D0..D3 = SEL1..SEL4（经与门 gate 各片 SCLK）、D4 = DIN，MSB 先行移入。
// 协议要点（手册 P4-P7）：数据 LSB 先行；DIN 仅在 SCLK 低电平变化；
// Start = SCLK 高时 DIN 下降沿；End = SCLK 高时 DIN 上升沿；上电须显存清零。

#include <stdint.h>

class Tm1640Chain {
 public:
  // GPIO 初始化 + 显存清零 + 设置亮度 + 显示开
  void begin();

  // 写单片显存（16 字节：GRID1..16，bit k = SEG(k+1) = 行 k）
  void writeRam(uint8_t chip, const uint8_t ram[16]);

  // 亮度 0..7（脉宽 1/16,2/16,4/16,10/16..14/16）
  void setBrightness(uint8_t level);

  // 显示开/关（关 = 0x80，开 = 0x90|亮度）
  void setDisplayOn(bool on);

 private:
  void shift595(uint8_t frame);  // MSB 先行移入并锁存
  void apply();                  // 595 输出 = sel_ | din_
  void setDin(bool level);
  void selectChip(uint8_t chip);
  void deselectAll();
  void sclkHigh();
  void sclkLow();
  void clkPulse();
  void start();
  void stop();
  void writeByte(uint8_t b);

  uint8_t sel_ = 0;    // 片选帧位（SEL1..4 = bit0..3）
  bool din_ = false;
  uint8_t level_ = 5;
};
