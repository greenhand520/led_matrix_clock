// 动画与日期行单元测试（主机侧 native 环境）：
// 数字垂直滚动合成、文本水平滚动起止、日期格式
// 显存约定 ram[模块][列] bit y
#include <unity.h>

#include "../../src/display/framebuffer.h"
#include "../../src/display/tm1640_map.h"
#include "../../src/rtc/datetime.h"
#include "../../src/ui/anim_digit.h"
#include "../../src/ui/date_line.h"
#include "../../src/ui/text_scroller.h"

void setUp() {}
void tearDown() {}

namespace {

const DigitSlot kSlot0 = {0, 0, FontKind::BIG, Color::RED};

void drawSlot0(FrameBuffer& fb, const DigitAnimator& anim) {
  DigitSlot slots[DigitAnimator::kSlots] = {
      kSlot0, kSlot0, kSlot0, kSlot0, kSlot0, kSlot0,
  };
  anim.draw(fb, slots);
}

}  // namespace

// 首帧直接落位：'1' 静态绘制（竖列 x=3 行 1/2/4/5）
static void test_digit_first_feed_static() {
  FrameBuffer fb;
  fb.clear();
  DigitAnimator anim;
  anim.feed(0, '1');
  TEST_ASSERT_FALSE(anim.animating());
  drawSlot0(fb, anim);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0x36, ram[0][3]);
}

// 滚动中段（p=2）：旧 '1' 下移 2 行（行 3/4/6/7），新 '2' 自上进入（行 0）
static void test_digit_scroll_mid() {
  FrameBuffer fb;
  fb.clear();
  DigitAnimator anim;
  anim.feed(0, '1');
  anim.feed(0, '2');
  TEST_ASSERT_TRUE(anim.animating());
  anim.step();
  anim.step();
  drawSlot0(fb, anim);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0xD9, ram[0][3]);
}

// 滚动结束（8 步）：仅新字形 '2' 落位（行 0/1/2/3/6）
static void test_digit_scroll_end() {
  FrameBuffer fb;
  fb.clear();
  DigitAnimator anim;
  anim.feed(0, '1');
  anim.feed(0, '2');
  for (int i = 0; i < 8; ++i) {
    anim.step();
  }
  TEST_ASSERT_FALSE(anim.animating());
  drawSlot0(fb, anim);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0x4F, ram[0][3]);
}

// 文本滚动：从 x=32 起步，尾部离开左边缘后停止
static void test_text_scroller() {
  TextScroller s;
  s.start("AB");  // 2 字符 × 6 列 = 12 列宽
  TEST_ASSERT_TRUE(s.running());
  TEST_ASSERT_EQUAL_INT16(32, s.x());
  for (int i = 0; i < 32; ++i) {
    s.step();
  }
  TEST_ASSERT_TRUE(s.running());  // x=0，尾部仍在屏内
  for (int i = 0; i < 12; ++i) {
    s.step();
  }
  TEST_ASSERT_FALSE(s.running());  // x=-12，尾部离开
}

// 日期文本三种顺序（wday=2 → MON）
static void test_date_text_orders() {
  const DateTime t = {2026, 10, 13, 12, 0, 0, 2};
  char buf[32];
  buildDateText(t, 0, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("2026-10-13 MON", buf);
  buildDateText(t, 1, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("13-10-2026 MON", buf);
  buildDateText(t, 2, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("10-13-2026 MON", buf);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_digit_first_feed_static);
  RUN_TEST(test_digit_scroll_mid);
  RUN_TEST(test_digit_scroll_end);
  RUN_TEST(test_text_scroller);
  RUN_TEST(test_date_text_orders);
  return UNITY_END();
}
