// RTC 纯逻辑单元测试（主机侧 native 环境）：BCD 编解码、时间校验、12h 转换
#include <unity.h>

#include "../../src/rtc/bcd.h"
#include "../../src/rtc/datetime.h"

void setUp() {}
void tearDown() {}

static void test_bcd_roundtrip() {
  TEST_ASSERT_EQUAL_HEX8(0x59, dec2bcd(59));
  TEST_ASSERT_EQUAL_HEX8(0x00, dec2bcd(0));
  TEST_ASSERT_EQUAL_UINT8(59, bcd2dec(0x59));
  TEST_ASSERT_EQUAL_UINT8(0, bcd2dec(0x00));
  for (uint8_t v = 0; v <= 99; ++v) {
    TEST_ASSERT_EQUAL_UINT8(v, bcd2dec(dec2bcd(v)));
  }
}

static void test_valid_datetime() {
  DateTime t = {2026, 10, 13, 23, 59, 59, 7};
  TEST_ASSERT_TRUE(validDateTime(t));

  t.year = 1999;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.year = 2026;

  t.mon = 13;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.mon = 0;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.mon = 10;

  t.day = 32;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.day = 13;

  t.hour = 24;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.hour = 23;

  t.wday = 0;
  TEST_ASSERT_FALSE(validDateTime(t));
  t.wday = 8;
  TEST_ASSERT_FALSE(validDateTime(t));
}

static void test_to12h() {
  bool pm = true;
  TEST_ASSERT_EQUAL_UINT8(12, to12h(0, &pm));
  TEST_ASSERT_FALSE(pm);
  TEST_ASSERT_EQUAL_UINT8(11, to12h(11, &pm));
  TEST_ASSERT_FALSE(pm);
  TEST_ASSERT_EQUAL_UINT8(12, to12h(12, &pm));
  TEST_ASSERT_TRUE(pm);
  TEST_ASSERT_EQUAL_UINT8(1, to12h(13, &pm));
  TEST_ASSERT_TRUE(pm);
  TEST_ASSERT_EQUAL_UINT8(11, to12h(23, &pm));
  TEST_ASSERT_TRUE(pm);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_bcd_roundtrip);
  RUN_TEST(test_valid_datetime);
  RUN_TEST(test_to12h);
  return UNITY_END();
}
