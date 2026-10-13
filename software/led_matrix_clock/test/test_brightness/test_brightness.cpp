// 自动亮度单元测试（主机侧 native 环境）：8 档映射、迟滞防抖
#include <unity.h>

#include "../../src/ui/brightness.h"

void setUp() {}
void tearDown() {}

// 暗环境 → 高档（亮屏）；亮环境 → 低档（暗屏）
static void test_mapping_direction() {
  AutoBrightness ab;
  ab.update(0);
  ab.update(0);
  TEST_ASSERT_EQUAL_UINT8(7, ab.update(0));     // 全暗 → 最亮档

  AutoBrightness ab2;
  ab2.update(1023);
  ab2.update(1023);
  TEST_ASSERT_EQUAL_UINT8(0, ab2.update(1023)); // 全亮 → 最暗档
}

// 迟滞：小幅波动不换档
static void test_hysteresis() {
  AutoBrightness ab;
  ab.update(512);
  ab.update(512);
  const uint8_t lv = ab.update(512);            // 目标 7-(512>>7)=3
  TEST_ASSERT_EQUAL_UINT8(3, lv);
  // ADC 小幅抖动（同档带）不换档
  for (int i = 0; i < 5; ++i) {
    TEST_ASSERT_EQUAL_UINT8(3, ab.update(520 + i));
  }
}

// 平滑后稳定换档：持续变暗 → 档位上升（EMA 收敛 + 连续 2 次同目标迟滞）
static void test_tracks_brightness_change() {
  AutoBrightness ab;
  ab.update(512);
  ab.update(512);
  ab.update(512);                               // 稳定在 3 档
  uint8_t lv = 3;
  for (int i = 0; i < 12; ++i) {
    lv = ab.update(0);                          // 转全暗，等 EMA 收敛并过迟滞
  }
  TEST_ASSERT_EQUAL_UINT8(7, lv);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mapping_direction);
  RUN_TEST(test_hysteresis);
  RUN_TEST(test_tracks_brightness_change);
  return UNITY_END();
}
