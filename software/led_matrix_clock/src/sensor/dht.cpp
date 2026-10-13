// DHT11/DHT22 驱动实现
#include "sensor/dht.h"

#include <Arduino.h>

namespace {
constexpr uint8_t kPinData = 5;
constexpr uint32_t kStartLowMs = 18;  // 起始信号低电平（DHT11 ≥18ms；DHT22 ≥1ms，兼容）
constexpr uint32_t kTimeoutUs = 120;  // 单电平等待超时
constexpr uint8_t kBits = 40;         // 湿度 8+8、温度 8+8、校验 8
constexpr uint8_t kDht11LockFrames = 4;  // 连续全零小数帧数 → 判定 DHT11

// 等待总线到指定电平；超时返回 false
bool waitLevel(bool level, uint32_t timeoutUs) {
  const uint32_t t0 = micros();
  while (digitalRead(kPinData) != level) {
    if (static_cast<uint32_t>(micros() - t0) > timeoutUs) {
      return false;
    }
  }
  return true;
}
}  // namespace

void Dht::begin() {
  pinMode(kPinData, INPUT_PULLUP);  // 空闲释放总线（外部 10k 上拉）
}

bool Dht::startSample(uint32_t nowMs) {
  if (state_ != State::IDLE) {
    return false;
  }
  pinMode(kPinData, OUTPUT);
  digitalWrite(kPinData, LOW);
  state_ = State::START_LOW;
  deadlineMs_ = nowMs + kStartLowMs;
  return true;
}

void Dht::tick(uint32_t nowMs) {
  if (state_ != State::START_LOW) {
    return;
  }
  if (static_cast<int32_t>(nowMs - deadlineMs_) < 0) {
    return;
  }
  readBurst();
  state_ = State::IDLE;
}

void Dht::readBurst() {
  uint8_t raw[5] = {0, 0, 0, 0, 0};
  bool ok = false;
  pinMode(kPinData, INPUT_PULLUP);  // 释放，传感器接管

  // 时序敏感窗口约 4ms：关中断一次性读完（设计文档 §2 权衡）
  noInterrupts();
  do {
    if (!waitLevel(false, kTimeoutUs)) break;  // 响应：80µs 低
    if (!waitLevel(true, kTimeoutUs)) break;   // 80µs 高
    if (!waitLevel(false, kTimeoutUs)) break;  // 进入数据位
    for (uint8_t i = 0; i < kBits; ++i) {
      if (!waitLevel(true, kTimeoutUs)) break;
      const uint32_t t0 = micros();
      if (!waitLevel(false, kTimeoutUs)) break;
      const uint32_t highUs = static_cast<uint32_t>(micros() - t0);
      raw[i / 8] = static_cast<uint8_t>(raw[i / 8] << 1);
      if (highUs > 40) {
        raw[i / 8] |= 1;  // 高电平 >40µs 为 1（26-28µs 为 0）
      }
    }
    ok = true;
  } while (false);
  interrupts();

  if (!ok || !dhtChecksumOk(raw)) {
    return;  // 超时/校验失败：保持上次值（R-ENV-03）
  }

  // 型号自动检测（粘滞）：DHT22 线索即锁定；全零小数帧累计 4 帧锁定 DHT11
  if (dhtFrameLooksDht22(raw)) {
    type_ = DhtType::DHT22;
    cleanFrames_ = 0;
  } else if (type_ == DhtType::UNKNOWN && ++cleanFrames_ >= kDht11LockFrames) {
    type_ = DhtType::DHT11;
  }

  DhtReading r;
  if (dhtDecodeFrame(raw, type_, r)) {
    tempTenths_ = r.tempTenths;
    humidity_ = r.humidity;
    valid_ = true;
  }
}
