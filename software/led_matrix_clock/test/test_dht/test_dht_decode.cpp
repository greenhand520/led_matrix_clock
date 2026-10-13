// DHT 解码/判别单元测试（主机侧 native 环境）：
// 覆盖 校验和、DHT11/DHT22 解码公式差异、负温、判别线索
#include <unity.h>

#include "../../src/sensor/dht_decode.h"

void setUp() {}
void tearDown() {}

static void test_checksum() {
  const uint8_t ok[5] = {45, 0, 25, 0, 70};  // 45+0+25+0 = 70
  const uint8_t bad[5] = {45, 0, 25, 0, 71};
  TEST_ASSERT_TRUE(dhtChecksumOk(ok));
  TEST_ASSERT_FALSE(dhtChecksumOk(bad));
}

// DHT11：45% / 25℃ 帧（小数字节恒 0）
static void test_decode_dht11() {
  const uint8_t raw[5] = {45, 0, 25, 0, 70};
  DhtReading r{};
  TEST_ASSERT_TRUE(dhtDecodeFrame(raw, DhtType::DHT11, r));
  TEST_ASSERT_EQUAL_INT(250, r.tempTenths);   // 25.0℃
  TEST_ASSERT_EQUAL_UINT8(45, r.humidity);
  TEST_ASSERT_FALSE(dhtFrameLooksDht22(raw));
}

// DHT22：25.3℃ / 45.6% 帧（温度 = (h<<8|l)，湿度 = (h<<8|l)/10）
static void test_decode_dht22() {
  // 湿度 raw16 = 456 → {0x01, 0xC8}；温度 raw16 = 253 → {0x00, 0xFD}
  const uint8_t raw[5] = {0x01, 0xC8, 0x00, 0xFD, 0xC6};  // 和 = 454 → 0xC6
  DhtReading r{};
  TEST_ASSERT_TRUE(dhtDecodeFrame(raw, DhtType::DHT22, r));
  TEST_ASSERT_EQUAL_INT(253, r.tempTenths);  // 25.3℃
  TEST_ASSERT_EQUAL_UINT8(45, r.humidity);   // 45%
  TEST_ASSERT_TRUE(dhtFrameLooksDht22(raw));
}

// DHT22 30.1℃：温度 raw16 = 301 = 0x012D —— 按 DHT11 公式会错算成 1.0℃
static void test_decode_dht22_tenths_encoding() {
  // 湿度 raw16 = 500 → {1, 0xF4}；温度 {0x01, 0x2D}；和 = 1+244+1+45 = 291 → 0x23
  const uint8_t raw[5] = {1, 0xF4, 0x01, 0x2D, 0x23};
  DhtReading r{};
  TEST_ASSERT_TRUE(dhtDecodeFrame(raw, DhtType::DHT22, r));
  TEST_ASSERT_EQUAL_INT(301, r.tempTenths);  // 30.1℃
  TEST_ASSERT_EQUAL_UINT8(50, r.humidity);
}

// DHT22 负温 -5.3℃：符号位在温度高字节 bit7
static void test_decode_dht22_negative() {
  // 湿度 {1, 0xF4}=500；温度 {0x80, 0x35}=符号+53；和 = 1+244+128+53 = 426 → 0xAA
  const uint8_t raw[5] = {1, 0xF4, 0x80, 0x35, 0xAA};
  DhtReading r{};
  TEST_ASSERT_TRUE(dhtDecodeFrame(raw, DhtType::DHT22, r));
  TEST_ASSERT_EQUAL_INT(-53, r.tempTenths);
  TEST_ASSERT_TRUE(dhtFrameLooksDht22(raw));
}

// 判别线索：越出 DHT11 量程同样判 DHT22
static void test_detect_clues() {
  const uint8_t humOver[5] = {101, 0, 25, 0, 126};
  const uint8_t tempOver[5] = {45, 0, 51, 0, 96};
  TEST_ASSERT_TRUE(dhtFrameLooksDht22(humOver));
  TEST_ASSERT_TRUE(dhtFrameLooksDht22(tempOver));
  // 校验失败帧不影响判别函数（纯比较），但解码拒绝
  const uint8_t bad[5] = {45, 0, 25, 0, 71};
  DhtReading r{};
  TEST_ASSERT_FALSE(dhtDecodeFrame(bad, DhtType::DHT22, r));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_checksum);
  RUN_TEST(test_decode_dht11);
  RUN_TEST(test_decode_dht22);
  RUN_TEST(test_decode_dht22_tenths_encoding);
  RUN_TEST(test_decode_dht22_negative);
  RUN_TEST(test_detect_clues);
  return UNITY_END();
}
