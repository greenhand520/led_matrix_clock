// 调度器单元测试（主机侧 native 环境，无硬件）
#include <unity.h>

#include "../../src/app/scheduler.h"

static int s_callsA = 0;
static int s_callsB = 0;

static void taskA(void*) { ++s_callsA; }
static void taskB(void*) { ++s_callsB; }

void setUp() {
  s_callsA = 0;
  s_callsB = 0;
}
void tearDown() {}

// 周期触发：到期执行一次，未到不执行
static void test_fires_at_period() {
  Scheduler s;
  TEST_ASSERT_TRUE(s.add(100, taskA));
  s.tick(0);  // 注册后首次 tick 立即执行
  TEST_ASSERT_EQUAL_INT(1, s_callsA);
  s.tick(99);
  TEST_ASSERT_EQUAL_INT(1, s_callsA);
  s.tick(100);
  TEST_ASSERT_EQUAL_INT(2, s_callsA);
}

// 长时间阻塞后不积压连跑，只补跑一次
static void test_no_burst_after_overrun() {
  Scheduler s;
  s.add(100, taskA);
  s.tick(0);
  s.tick(1000);
  TEST_ASSERT_EQUAL_INT(2, s_callsA);
  s.tick(1001);
  TEST_ASSERT_EQUAL_INT(2, s_callsA);
}

// 多任务各按各的周期独立触发
static void test_multiple_tasks_independent() {
  Scheduler s;
  s.add(100, taskA);
  s.add(200, taskB);
  s.tick(0);
  TEST_ASSERT_EQUAL_INT(1, s_callsA);
  TEST_ASSERT_EQUAL_INT(1, s_callsB);
  s.tick(200);
  TEST_ASSERT_EQUAL_INT(2, s_callsA);
  TEST_ASSERT_EQUAL_INT(2, s_callsB);
}

// millis() 回绕仍正确触发
static void test_tick_wraparound() {
  Scheduler s;
  s.add(100, taskA);
  s.tick(0xFFFFFFF0u);
  TEST_ASSERT_EQUAL_INT(1, s_callsA);
  s.tick(0x00000054u);  // 回绕后到达周期
  TEST_ASSERT_EQUAL_INT(2, s_callsA);
}

// 非法注册被拒绝，任务槽满后拒绝
static void test_reject_invalid_and_full() {
  Scheduler s;
  TEST_ASSERT_FALSE(s.add(0, taskA));
  TEST_ASSERT_FALSE(s.add(100, nullptr));
  TEST_ASSERT_TRUE(s.add(100, taskA));
  for (int i = 0; i < 11; ++i) {
    TEST_ASSERT_TRUE(s.add(100, taskB));
  }
  TEST_ASSERT_EQUAL_UINT8(12, s.count());
  TEST_ASSERT_FALSE(s.add(100, taskA));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_fires_at_period);
  RUN_TEST(test_no_burst_after_overrun);
  RUN_TEST(test_multiple_tasks_independent);
  RUN_TEST(test_tick_wraparound);
  RUN_TEST(test_reject_invalid_and_full);
  return UNITY_END();
}
