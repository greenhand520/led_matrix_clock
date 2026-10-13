// 帧缓冲单元测试（主机侧 native 环境，无硬件）：
// 覆盖 显存转置映射、双色合成、方向变换、越界裁剪、字形落点与字距
// 显存约定（硬件设计）：ram[模块][列 c] bit y = 行 y；红列 c → ram[m][c]，绿列 c → ram[m][8+c]
#include <unity.h>

#include "../../src/display/framebuffer.h"
#include "../../src/display/font_data.h"

void setUp() {}
void tearDown() {}

static void test_compose_mapping() {
  FrameBuffer fb;
  fb.clear();
  fb.setPixel(0, 0, Color::RED);    // 模块0 列0 行0
  fb.setPixel(7, 7, Color::GREEN);  // 模块0 列7 行7
  fb.setPixel(8, 3, Color::RED);    // 模块1 列0 行3
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x01, ram[0][0]);   // 列0 bit0
  TEST_ASSERT_EQUAL_HEX8(0x80, ram[0][15]);  // 绿列7 bit7
  TEST_ASSERT_EQUAL_HEX8(0x08, ram[1][0]);   // 列0 bit3
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[1][3]);
}

// ORANGE = 双色同亮：(5,2) → 列5 bit2，红/绿平面同亮
static void test_orange_sets_both_planes() {
  FrameBuffer fb;
  fb.clear();
  fb.setPixel(5, 2, Color::ORANGE);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x04, ram[0][5]);
  TEST_ASSERT_EQUAL_HEX8(0x04, ram[0][13]);
}

// clear(RED)：红平面全亮、绿平面全灭
static void test_clear_color() {
  FrameBuffer fb;
  fb.clear(Color::RED);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  for (int m = 0; m < 4; ++m) {
    for (int i = 0; i < 8; ++i) {
      TEST_ASSERT_EQUAL_HEX8(0xFF, ram[m][i]);
      TEST_ASSERT_EQUAL_HEX8(0x00, ram[m][8 + i]);
    }
  }
}

// 旋转 180°：(0,0) → (31,7)；(31,7) → (0,0)
static void test_rotate_180() {
  FrameBuffer fb;
  fb.clear();
  fb.setRotate(RotateMode::ROT180);
  fb.setPixel(0, 0, Color::RED);
  fb.setPixel(31, 7, Color::GREEN);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x80, ram[3][7]);  // 红落到 (31,7)：列7 bit7
  TEST_ASSERT_EQUAL_HEX8(0x01, ram[0][8]);  // 绿落到 (0,0)：绿列0 bit0
}

// 水平镜像：(0,4) → (31,4)；垂直镜像：(2,0) → (2,7)
static void test_mirror_h_v() {
  FrameBuffer fb;
  fb.clear();
  fb.setRotate(RotateMode::MIRROR_H);
  fb.setPixel(0, 4, Color::RED);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x10, ram[3][7]);  // (31,4)：列7 bit4

  fb.clear();
  fb.setRotate(RotateMode::MIRROR_V);
  fb.setPixel(2, 0, Color::RED);
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x80, ram[0][2]);  // (2,7)：列2 bit7
}

// 越界裁剪：不落点也不越界访问
static void test_clip_out_of_bounds() {
  FrameBuffer fb;
  fb.clear();
  fb.setPixel(-1, 0, Color::RED);
  fb.setPixel(32, 0, Color::RED);
  fb.setPixel(0, -1, Color::RED);
  fb.setPixel(0, 8, Color::RED);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  for (int m = 0; m < 4; ++m) {
    for (int i = 0; i < 16; ++i) {
      TEST_ASSERT_EQUAL_HEX8(0x00, ram[m][i]);
    }
  }
}

// 大数字 '1'（七段样式）：仅右竖列 x=3，行 1/2/4/5
static void test_big_digit_one() {
  FrameBuffer fb;
  fb.clear();
  fb.drawChar('1', 0, 0, Color::RED, FontKind::BIG);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x36, ram[0][3]);  // 列3 bit1/2/4/5
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[0][2]);
}

// 小字字距 4：'00' 首字 x=24（模块3 列0-2）、次字 x=28（列4-6）；字形行 [7,5,0,5,7]
static void test_small_text_pitch() {
  FrameBuffer fb;
  fb.clear();
  fb.drawText("00", 24, 2, Color::GREEN, FontKind::SMALL);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  TEST_ASSERT_EQUAL_HEX8(0x6C, ram[3][8 + 0]);  // 列0：行2/3/5/6
  TEST_ASSERT_EQUAL_HEX8(0x44, ram[3][8 + 1]);  // 列1：行2/6
  TEST_ASSERT_EQUAL_HEX8(0x6C, ram[3][8 + 2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, ram[3][8 + 3]);  // 字距列
  TEST_ASSERT_EQUAL_HEX8(0x6C, ram[3][8 + 4]);  // 次字起点列4
}

// glcdfont 字距 6：同字符两连画应落在 x=0..4 与 x=6..10，逐像素一致
static void test_glcd_text_pitch() {
  FrameBuffer fb;
  fb.clear();
  fb.drawText("AA", 0, 0, Color::RED, FontKind::GLCD);
  uint8_t ram[FrameBuffer::kModules][16];
  fb.composeToRam(ram);
  for (int row = 0; row < 8; ++row) {
    for (int c = 0; c < 5; ++c) {
      const bool a = (ram[c / 8][c % 8] >> row) & 1;
      const int c2 = c + 6;
      const bool b = (ram[c2 / 8][c2 % 8] >> row) & 1;
      TEST_ASSERT_TRUE(a == b);
    }
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_compose_mapping);
  RUN_TEST(test_orange_sets_both_planes);
  RUN_TEST(test_clear_color);
  RUN_TEST(test_rotate_180);
  RUN_TEST(test_mirror_h_v);
  RUN_TEST(test_clip_out_of_bounds);
  RUN_TEST(test_big_digit_one);
  RUN_TEST(test_small_text_pitch);
  RUN_TEST(test_glcd_text_pitch);
  return UNITY_END();
}
