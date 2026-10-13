#pragma once
// DHT11/DHT22(AM2302) 单总线温湿度驱动（DATA=GPIO5、3.3V 供电/上拉，见硬件设计）
// 型号自动检测（粘滞）：解码公式随型号不同（见 dht_decode.h），判别后锁定；
//   - 帧判别线索（小数字节非零/越出 DHT11 量程/负温）→ DHT22
//   - 连续 4 帧全零小数且在 DHT11 量程 → DHT11
// 采样事务：起始低电平 18ms 由状态机计时（非阻塞，对 DHT11/DHT22 均有效）；
// 总线读取为约 4ms 的关中断突发窗口。
#include <stdint.h>

#include "sensor/dht_decode.h"

class Dht {
 public:
  void begin();

  // 启动一次采样事务；空闲才受理（上一事务未完返回 false）
  bool startSample(uint32_t nowMs);

  // 推进状态机：起始低电平到点后触发读取突发
  void tick(uint32_t nowMs);

  bool sampling() const { return state_ != State::IDLE; }

  // 最近一次解析是否有效（校验和通过）；失败时数值保持上次有效值（R-ENV-03）
  bool valid() const { return valid_; }

  DhtType type() const { return type_; }  // 自动检测结果
  int16_t temperatureTenths() const { return tempTenths_; }  // 0.1℃
  uint8_t humidity() const { return humidity_; }             // %

 private:
  void readBurst();  // 关中断读 40bit + 校验/判别/解析

  enum class State : uint8_t { IDLE, START_LOW };
  State state_ = State::IDLE;
  uint32_t deadlineMs_ = 0;
  DhtType type_ = DhtType::UNKNOWN;
  uint8_t cleanFrames_ = 0;  // 判别期：全零小数帧计数
  int16_t tempTenths_ = 0;
  uint8_t humidity_ = 0;
  bool valid_ = false;
};
