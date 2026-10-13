// 状态覆盖层单元测试（主机侧 native 环境）：sticky/定时语义、替换与清除
#include <unity.h>

#include "../../src/display/framebuffer.h"
#include "../../src/ui/overlay.h"

void setUp() {}
void tearDown() {}

// sticky：持续有效直到被替换/清除（Wi-Fi/Setup 语义）
static void test_sticky_stays_active() {
  StatusOverlay ov;
  ov.show("Setup", 1000, true);
  TEST_ASSERT_TRUE(ov.active(1000));
  TEST_ASSERT_TRUE(ov.active(999999));
  ov.clear();
  TEST_ASSERT_FALSE(ov.active(1000));
}

// 定时：2s 后消失（OK!!/Err!/RTC! 语义）
static void test_timed_expires() {
  StatusOverlay ov;
  ov.show("OK!!", 1000, false);
  TEST_ASSERT_TRUE(ov.active(1000));
  TEST_ASSERT_TRUE(ov.active(2900));   // 1.9s 仍有效
  TEST_ASSERT_FALSE(ov.active(3001));  // 2.01s 消失
}

// show 替换当前提示（结果覆盖 Wi-Fi）
static void test_show_replaces() {
  StatusOverlay ov;
  ov.show("Wi-Fi", 1000, true);
  ov.show("Err!", 5000, false);
  TEST_ASSERT_TRUE(ov.active(5500));
  TEST_ASSERT_FALSE(ov.active(7500));  // 替换后按新提示计时
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sticky_stays_active);
  RUN_TEST(test_timed_expires);
  RUN_TEST(test_show_replaces);
  return UNITY_END();
}
