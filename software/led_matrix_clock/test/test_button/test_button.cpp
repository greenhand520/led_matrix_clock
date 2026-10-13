// 按键状态机单元测试（主机侧 native 环境）：消抖、短按、长按分级
#include <unity.h>

#include "../../src/ui/button.h"

void setUp() {}
void tearDown() {}

// 消抖：短于 50ms 的抖动不产生事件
static void test_debounce_bounce() {
  Button b;
  TEST_ASSERT_TRUE(b.poll(true, 0) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(false, 10) == KeyEvt::NONE);   // 抖动回弹
  TEST_ASSERT_TRUE(b.poll(true, 20) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(false, 30) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(false, 90) == KeyEvt::NONE);   // 稳定松开，无短按（从未稳定按下）
}

// 短按：稳定按下后 <2s 松开
static void test_short_press() {
  Button b;
  TEST_ASSERT_TRUE(b.poll(true, 0) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(true, 50) == KeyEvt::NONE);    // 稳定按下
  TEST_ASSERT_TRUE(b.poll(true, 200) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(false, 300) == KeyEvt::NONE);  // 松开沿
  TEST_ASSERT_TRUE(b.poll(false, 350) == KeyEvt::SHORT); // 消抖确认 → 短按
}

// 长按 2s：到点触发一次，松开不再发短按
static void test_long_2s() {
  Button b;
  b.poll(true, 0);
  b.poll(true, 50);
  TEST_ASSERT_TRUE(b.poll(true, 2050) == KeyEvt::LONG_2S);
  TEST_ASSERT_TRUE(b.poll(true, 2100) == KeyEvt::NONE);  // 不重复触发
  b.poll(false, 2200);
  TEST_ASSERT_TRUE(b.poll(false, 2250) == KeyEvt::NONE); // 无 SHORT
}

// 长按 5s：先 LONG_2S 再 LONG_5S，各一次
static void test_long_5s() {
  Button b;
  b.poll(true, 0);
  b.poll(true, 50);
  TEST_ASSERT_TRUE(b.poll(true, 2050) == KeyEvt::LONG_2S);
  TEST_ASSERT_TRUE(b.poll(true, 4900) == KeyEvt::NONE);
  TEST_ASSERT_TRUE(b.poll(true, 5050) == KeyEvt::LONG_5S);
  TEST_ASSERT_TRUE(b.poll(true, 5100) == KeyEvt::NONE);  // 不重复
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_debounce_bounce);
  RUN_TEST(test_short_press);
  RUN_TEST(test_long_2s);
  RUN_TEST(test_long_5s);
  return UNITY_END();
}
