// PAGE_ENV 渲染与舒适度着色单元测试（主机侧 native 环境）：
// 布局稿 v0.4 §3（7 槽位 27 列）；舒适度边界见需求 R-ENV-05
// 显存约定 ram[模块][列] bit y
#include <unity.h>

#include "../../src/display/framebuffer.h"
#include "../../src/display/tm1640_map.h"
#include "../../src/ui/comfort.h"
#include "../../src/ui/page_env.h"

void setUp() {}
void tearDown() {}

namespace {

void renderTo(FrameBuffer& fb, int16_t tenths, uint8_t hum, bool tOk, bool hOk) {
  fb.clear();
  page_env::render(fb, tenths, hum, tOk, hOk);
}

}  // namespace

// 舒适度边界（需求 R-ENV-05 用例：17/18/26/27℃、39/40/70/71%）
static void test_comfort_boundaries() {
  TEST_ASSERT_TRUE(comfortColorTemp(119) == Color::RED);
  TEST_ASSERT_TRUE(comfortColorTemp(120) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorTemp(179) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorTemp(180) == Color::GREEN);
  TEST_ASSERT_TRUE(comfortColorTemp(260) == Color::GREEN);
  TEST_ASSERT_TRUE(comfortColorTemp(261) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorTemp(300) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorTemp(301) == Color::RED);

  TEST_ASSERT_TRUE(comfortColorHum(29) == Color::RED);
  TEST_ASSERT_TRUE(comfortColorHum(39) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorHum(40) == Color::GREEN);
  TEST_ASSERT_TRUE(comfortColorHum(70) == Color::GREEN);
  TEST_ASSERT_TRUE(comfortColorHum(71) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorHum(80) == Color::ORANGE);
  TEST_ASSERT_TRUE(comfortColorHum(81) == Color::RED);
}

// 25.3°C / 45%（双绿）：槽位落点与颜色（红平面应为空）
static void test_render_layout_and_color() {
  FrameBuffer fb;
  renderTo(fb, 253, 45, true, true);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  // 温度 '2' @x=0（模块0 列0-2）、小数点 @x=7、'°C' @x=12（模块1 列4）
  TEST_ASSERT_EQUAL_HEX8(0x74, ram[0][8 + 0]);   // '2' 列0（绿）
  TEST_ASSERT_EQUAL_HEX8(0x5C, ram[0][8 + 2]);   // '2' 列2（绿）
  TEST_ASSERT_EQUAL_HEX8(0x40, ram[0][8 + 7]);   // 小数点 y=6
  TEST_ASSERT_EQUAL_HEX8(0x74, ram[1][8 + 4]);   // °C 列
  // 湿度 '4' @x=16（模块2 列0）、'%' @x=24（模块3 列0）
  TEST_ASSERT_EQUAL_HEX8(0x18, ram[2][8 + 0]);   // '4' 列0
  TEST_ASSERT_EQUAL_HEX8(0x64, ram[3][8 + 0]);   // '%' 列0
  // 25.3℃ 绿 / 45% 绿 → 红平面对应列为空
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[1][4]);
}

// 17.9℃ / 39%（双橙）：红绿平面同亮
static void test_render_orange() {
  FrameBuffer fb;
  renderTo(fb, 179, 39, true, true);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0x28, ram[0][2]);    // 温度 '1' 列2（红）
  TEST_ASSERT_EQUAL_HEX8(0x28, ram[0][8 + 2]);  // 同列绿（橙=双色同亮）
  TEST_ASSERT_EQUAL_HEX8(0x54, ram[2][0]);    // 湿度 '3' 列0 红
  TEST_ASSERT_EQUAL_HEX8(0x54, ram[2][8]);    // 湿度 '3' 列0 绿
}

// 采集失败：整组 "--"（红），单位与小数点不画
static void test_render_invalid_shows_dashes() {
  FrameBuffer fb;
  renderTo(fb, 0, 0, false, false);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[0][0]);   // 温度十位 '-'（y=4）
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[0][4]);   // 温度个位 '-'
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][7]);   // 小数点不画
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[1][4]);   // °C 不画
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[2][0]);   // 湿度十位 '-'
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[2][4]);   // 湿度个位 '-'
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[3][0]);   // % 不画
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][8]);   // 绿平面为空（'--' 为红）
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[2][8]);   // 同上（湿度组）
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[0][0]);   // 红平面已有十位 '-'（复核）
}

// 负温度：'-' 占十位槽（DHT22 场景）
static void test_render_negative_temp() {
  FrameBuffer fb;
  renderTo(fb, -53, 45, true, true);
  uint8_t ram[FrameBuffer::kModules][16];
  composeTm1640Ram(fb, ram);
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[0][0]);   // '-'（y=4，红）
  TEST_ASSERT_EQUAL_HEX8(0x5C, ram[0][4]);   // 个位 '5'（红，列4；-5.3℃ 属红色区间）
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comfort_boundaries);
  RUN_TEST(test_render_layout_and_color);
  RUN_TEST(test_render_orange);
  RUN_TEST(test_render_invalid_shows_dashes);
  RUN_TEST(test_render_negative_temp);
  return UNITY_END();
}
