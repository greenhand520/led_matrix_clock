#pragma once
// DHT11/DHT22 协议帧解码与型号判别（纯逻辑，可主机侧单测）
//
// 两型号 40bit 帧格式相同：湿度 8+8、温度 8+8、校验 8；差异在小数字节语义：
//   DHT11：小数字节恒 0；温度 = int×10+dec，湿度 = int
//   DHT22：温度 = (h&0x7F)<<8 | l（0.1℃，符号在 h bit7）；湿度 = (h<<8|l)/10
// 解码公式不同（DHT22 的 30.1℃ 帧按 DHT11 解会得 1.0℃），故必须判别型号。
#include <stdint.h>

enum class DhtType : uint8_t { UNKNOWN, DHT11, DHT22 };

struct DhtReading {
  int16_t tempTenths;  // 0.1℃（253 = 25.3℃）
  uint8_t humidity;    // %
};

inline bool dhtChecksumOk(const uint8_t raw[5]) {
  return static_cast<uint8_t>(raw[0] + raw[1] + raw[2] + raw[3]) == raw[4];
}

// 判别线索：DHT22 小数字节几乎总非零；且可能越出 DHT11 量程（0..50℃ / 0..100%）或带负温符号位
inline bool dhtFrameLooksDht22(const uint8_t raw[5]) {
  if (raw[1] != 0 || raw[3] != 0) {
    return true;
  }
  if (raw[0] > 100) {
    return true;
  }
  if ((raw[2] & 0x80) != 0) {
    return true;
  }
  return (raw[2] & 0x7F) > 50;
}

// 按型号解码；type=UNKNOWN 时按 DHT11 语义（判别期临时值）；校验失败返回 false
inline bool dhtDecodeFrame(const uint8_t raw[5], DhtType type, DhtReading& out) {
  if (!dhtChecksumOk(raw)) {
    return false;
  }
  if (type == DhtType::DHT22) {
    int16_t t = static_cast<int16_t>(((raw[2] & 0x7F) << 8) | raw[3]);
    if ((raw[2] & 0x80) != 0) {
      t = static_cast<int16_t>(-t);
    }
    out.tempTenths = t;
    const uint16_t h = static_cast<uint16_t>((raw[0] << 8) | raw[1]);
    out.humidity = static_cast<uint8_t>(h > 1000 ? 100 : h / 10);
  } else {
    out.tempTenths = static_cast<int16_t>(raw[2] * 10 + raw[3]);
    out.humidity = raw[0];
  }
  return true;
}
