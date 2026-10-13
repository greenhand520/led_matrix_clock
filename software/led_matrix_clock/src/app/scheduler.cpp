// 协作式定时节拍器实现
#include "app/scheduler.h"

bool Scheduler::add(uint32_t periodMs, TaskFn fn, void* ctx) {
  if (fn == nullptr || periodMs == 0 || count_ >= kMaxTasks) {
    return false;
  }
  slots_[count_].fn = fn;
  slots_[count_].ctx = ctx;
  slots_[count_].periodMs = periodMs;
  slots_[count_].nextMs = 0;
  slots_[count_].pending = true;  // 首次 tick 立即执行（不依赖当前时间）
  ++count_;
  return true;
}

void Scheduler::tick(uint32_t nowMs) {
  for (uint8_t i = 0; i < count_; ++i) {
    Slot& s = slots_[i];
    // pending：新注册任务首跑；否则用有符号差值判断到期，兼容 millis() 约 49.7 天的回绕
    if (s.pending || static_cast<int32_t>(nowMs - s.nextMs) >= 0) {
      s.pending = false;
      s.fn(s.ctx);
      s.nextMs = nowMs + s.periodMs;  // 从执行时刻顺延，避免阻塞后补跑堆积
    }
  }
}
