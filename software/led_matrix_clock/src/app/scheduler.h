#pragma once
// 协作式定时节拍器：周期任务全部用截止时间驱动，禁止 delay() 阻塞
// 时间参数由调用方传入（millis()），纯逻辑、可在主机侧单测

#include <stdint.h>

class Scheduler {
 public:
  using TaskFn = void (*)(void* ctx);

  // 注册周期任务（periodMs>0，fn 非空）；任务槽满或参数非法返回 false
  // 首次 tick 会立即执行一次，用于任务初始化
  bool add(uint32_t periodMs, TaskFn fn, void* ctx = nullptr);

  // 推进调度：到期任务执行一次，随后从执行时刻顺延一个周期（阻塞后不积压连跑）
  void tick(uint32_t nowMs);

  uint8_t count() const { return count_; }

 private:
  static const uint8_t kMaxTasks = 12;

  struct Slot {
    TaskFn fn;
    void* ctx;
    uint32_t periodMs;
    uint32_t nextMs;
    bool pending;  // 新注册任务：保证首次 tick 立即执行，与当前时间无关
  };

  Slot slots_[kMaxTasks];
  uint8_t count_ = 0;
};
