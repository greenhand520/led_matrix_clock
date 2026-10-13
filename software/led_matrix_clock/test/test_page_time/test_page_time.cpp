// PAGE_TIME 渲染单元测试（主机侧 native 环境）：
// 坐标/字号按布局稿 v0.4 §2；显存约定 ram[模块][列] bit y
#include <unity.h>

#include "../../src/config/config_schema.h"
#include "../../src/display/framebuffer.h"
#include "../../src/rtc/datetime.h"
#include "../../src/ui/page_time.h"

void setUp() {}
void tearDown() {}

namespace {

// 2026-10-13 12:45:07 星期一
const DateTime kT = {2026, 10, 13, 12, 45, 07, 1};

void renderTo(FrameBuffer& fb, const DateTime& t, const Config& cfg) {
  fb.clear();
  page_time::render(fb, t, cfg);
}

}  // namespace

// 24h：大字 '1' 落 H1（x=0..3 的竖列 x=3 行 1/2/4/5），秒 '0' 落 S1（模块3 列0）
static void test_render_24h_layout() {
  Config cfg;  // hour_format=24, colon_blink=1
  FrameBuffer fb;
  renderTo(fb, kT, cfg);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x36, ram[0][3]);       // H1 '1' 竖列（行1/2/4/5）
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][0]);       // H1 为 '1'，列0 无像素
  TEST_ASSERT_EQUAL_HEX8(0x6C, ram[3][8 + 0]);   // S1 '0'（绿，列0 行2/3/5/6）
  TEST_ASSERT_EQUAL_HEX8(0x04, ram[3][8 + 4]);   // S2 '7' 顶横（列4 行2）
}

// 冒号：sec=07 奇数 → 闪灭；colon_blink=0 → 常亮
static void test_colon_blink() {
  Config cfg;
  FrameBuffer fb;
  renderTo(fb, kT, cfg);  // sec=7 奇数
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[1][2]);

  cfg.colon_blink = 0;
  renderTo(fb, kT, cfg);
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x14, ram[1][2]);  // y=2 与 y=4 两点

  DateTime t2 = kT;
  t2.sec = 6;
  cfg.colon_blink = 1;
  renderTo(fb, t2, cfg);
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x14, ram[1][2]);  // sec 偶数亮
}

// 12h：小时前导 0 不点亮（05 → H1 空白），AM 角标绿（红平面灭）
static void test_12h_leading_zero_and_am_chip() {
  Config cfg;
  cfg.hour_format = 12;
  DateTime t = kT;
  t.hour = 5;  // 12h 显示 05? 前导 0 不点亮 → 仅 H2 '5'
  FrameBuffer fb;
  renderTo(fb, t, cfg);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][0]);  // H1 '0' 未画
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][3]);
  // AM 角标（x=28..30, y=0..1）：绿亮红灭
  TEST_ASSERT_EQUAL_HEX8(0x03, ram[3][8 + 4] & 0x03);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[3][4] & 0x03);
}

// 12h PM：hour=13 → 01 PM，角标 ORANGE（红绿同亮），H1 空白
static void test_12h_pm_chip_orange() {
  Config cfg;
  cfg.hour_format = 12;
  DateTime t = kT;
  t.hour = 13;
  FrameBuffer fb;
  renderTo(fb, t, cfg);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][3]);
  TEST_ASSERT_EQUAL_HEX8(0x03, ram[3][4] & 0x03);   // 红平面
  TEST_ASSERT_EQUAL_HEX8(0x03, ram[3][8 + 4] & 0x03);  // 绿平面
}

// 24h 前导零点亮：hour=05 → H1 '0'（4x7 '0' = 行 0/1/2/4/5/6 有列0 像素）
static void test_24h_leading_zero_lit() {
  Config cfg;  // 24h
  DateTime t = kT;
  t.hour = 5;
  FrameBuffer fb;
  renderTo(fb, t, cfg);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x77, ram[0][0]);  // H1 '0' 列0 行0/1/2/4/5/6
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_render_24h_layout);
  RUN_TEST(test_colon_blink);
  RUN_TEST(test_12h_leading_zero_and_am_chip);
  RUN_TEST(test_12h_pm_chip_orange);
  RUN_TEST(test_24h_leading_zero_lit);
  return UNITY_END();
}
