#pragma once
// 按键状态机（纯逻辑，可主机侧单测）：GPIO0 按键（低有效），消抖 50ms
// 事件语义（需求 R-KEY-01/02）：
//   SHORT   —— 按下 <2s 松开（切换显示页并重置轮换计时）
//   LONG_2S —— 持续按住到 2s（触发一次；配网，P6 接线）
//   LONG_5S —— 持续按住到 5s（触发一次；恢复出厂）
#include <stdint.h>

enum class KeyEvt : uint8_t { NONE, SHORT, LONG_2S, LONG_5S };

class Button {
 public:
  // 每 10ms 调用一次；pressed = 当前原始电平（true 为按下）
  KeyEvt poll(bool pressed, uint32_t nowMs) {
    // 消抖：原始电平变化需保持 kDebounceMs 才被采信
    if (pressed != lastRaw_) {
      lastRaw_ = pressed;
      lastEdgeMs_ = nowMs;
      return KeyEvt::NONE;
    }
    if (pressed != stablePressed_ &&
        static_cast<uint32_t>(nowMs - lastEdgeMs_) >= kDebounceMs) {
      stablePressed_ = pressed;
      if (pressed) {
        pressStartMs_ = nowMs;
        fired_ = 0;
      } else {
        if (fired_ == 0) {
          return KeyEvt::SHORT;  // 松开且未触发长按 → 短按
        }
        fired_ = 0;
        return KeyEvt::NONE;
      }
    }

    // 长按分级（按下期间各触发一次）
    if (stablePressed_) {
      const uint32_t held = nowMs - pressStartMs_;
      if (fired_ < 2 && held >= kLong5sMs) {
        fired_ = 2;
        return KeyEvt::LONG_5S;
      }
      if (fired_ < 1 && held >= kLong2sMs) {
        fired_ = 1;
        return KeyEvt::LONG_2S;
      }
    }
    return KeyEvt::NONE;
  }

 private:
  static constexpr uint32_t kDebounceMs = 50;
  static constexpr uint32_t kLong2sMs = 2000;
  static constexpr uint32_t kLong5sMs = 5000;

  bool stablePressed_ = false;
  bool lastRaw_ = false;
  uint32_t lastEdgeMs_ = 0;
  uint32_t pressStartMs_ = 0;
  uint8_t fired_ = 0;  // 0=无 1=已发 LONG_2S 2=已发 LONG_5S
};
