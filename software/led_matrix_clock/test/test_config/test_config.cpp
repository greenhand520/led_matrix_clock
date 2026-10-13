// 配置序列化单元测试（主机侧 native 环境，无硬件）
#include <ArduinoJson.h>
#include <unity.h>

#include "../../src/config/config_schema.h"

void setUp() {}
void tearDown() {}

// 默认值与需求文档 §8 配置项清单一致
static void test_defaults_match_requirements() {
  const Config cfg;
  TEST_ASSERT_EQUAL_INT(480, cfg.tz_offset_min);
  TEST_ASSERT_EQUAL_STRING("time1.aliyun.com", cfg.ntp_server);
  TEST_ASSERT_EQUAL_UINT8(6, cfg.ntp_interval_h);
  TEST_ASSERT_EQUAL_UINT8(24, cfg.hour_format);
  TEST_ASSERT_EQUAL_UINT8(0, cfg.date_order);
  TEST_ASSERT_EQUAL_UINT8(ROTATE_NORMAL, cfg.rotate_mode);
  TEST_ASSERT_EQUAL_UINT8(1, cfg.colon_blink);
  TEST_ASSERT_EQUAL_UINT8(0, cfg.brightness_mode);
  TEST_ASSERT_EQUAL_UINT8(5, cfg.brightness_level);
  TEST_ASSERT_EQUAL_UINT8(PAGE_DEFAULT_TIME, cfg.default_page);
  TEST_ASSERT_EQUAL_UINT16(15, cfg.page_switch_s);
  TEST_ASSERT_EQUAL_UINT8(3, cfg.env_sample_s);
  TEST_ASSERT_EQUAL_UINT16(30, cfg.date_scroll_min);
}

// JSON 往返：toJson -> fromJson 字段全部一致
static void test_json_roundtrip() {
  Config src;
  src.tz_offset_min = -300;
  snprintf(src.ntp_server, sizeof(src.ntp_server), "%s", "pool.ntp.org");
  src.ntp_interval_h = 12;
  src.hour_format = 12;
  src.date_order = 2;
  src.rotate_mode = ROTATE_MIRROR_V;
  src.colon_blink = 0;
  src.brightness_mode = 1;
  src.brightness_level = 7;
  src.default_page = PAGE_DEFAULT_ENV;
  src.page_switch_s = 20;
  src.env_sample_s = 5;
  src.date_scroll_min = 60;

  JsonDocument doc;
  config::toJson(src, doc);

  Config dst;
  TEST_ASSERT_TRUE(config::fromJson(dst, doc));
  TEST_ASSERT_EQUAL_INT(src.tz_offset_min, dst.tz_offset_min);
  TEST_ASSERT_EQUAL_STRING(src.ntp_server, dst.ntp_server);
  TEST_ASSERT_EQUAL_UINT8(src.ntp_interval_h, dst.ntp_interval_h);
  TEST_ASSERT_EQUAL_UINT8(src.hour_format, dst.hour_format);
  TEST_ASSERT_EQUAL_UINT8(src.date_order, dst.date_order);
  TEST_ASSERT_EQUAL_UINT8(src.rotate_mode, dst.rotate_mode);
  TEST_ASSERT_EQUAL_UINT8(src.colon_blink, dst.colon_blink);
  TEST_ASSERT_EQUAL_UINT8(src.brightness_mode, dst.brightness_mode);
  TEST_ASSERT_EQUAL_UINT8(src.brightness_level, dst.brightness_level);
  TEST_ASSERT_EQUAL_UINT8(src.default_page, dst.default_page);
  TEST_ASSERT_EQUAL_UINT16(src.page_switch_s, dst.page_switch_s);
  TEST_ASSERT_EQUAL_UINT8(src.env_sample_s, dst.env_sample_s);
  TEST_ASSERT_EQUAL_UINT16(src.date_scroll_min, dst.date_scroll_min);
}

// schema 版本不符：整份回退默认值
static void test_schema_mismatch_keeps_defaults() {
  JsonDocument doc;
  doc["schema"] = 999;
  doc["tz_offset_min"] = -300;

  Config cfg;
  TEST_ASSERT_FALSE(config::fromJson(cfg, doc));
  TEST_ASSERT_EQUAL_INT(480, cfg.tz_offset_min);
}

// 空文档：视为无配置
static void test_empty_doc_keeps_defaults() {
  JsonDocument doc;
  Config cfg;
  TEST_ASSERT_FALSE(config::fromJson(cfg, doc));
  TEST_ASSERT_EQUAL_UINT16(15, cfg.page_switch_s);
}

// 缺字段：沿用默认值，其余字段正常填充
static void test_partial_doc_uses_defaults_for_missing() {
  JsonDocument doc;
  doc["schema"] = config::kSchema;
  doc["page_switch_s"] = 25;

  Config cfg;
  TEST_ASSERT_TRUE(config::fromJson(cfg, doc));
  TEST_ASSERT_EQUAL_UINT16(25, cfg.page_switch_s);
  TEST_ASSERT_EQUAL_UINT8(3, cfg.env_sample_s);
  TEST_ASSERT_EQUAL_STRING("time1.aliyun.com", cfg.ntp_server);
}

// 越界值收敛到合法范围
static void test_out_of_range_clamped() {
  JsonDocument doc;
  doc["schema"] = config::kSchema;
  doc["brightness_level"] = 200;
  doc["page_switch_s"] = 0;
  doc["hour_format"] = 7;
  doc["env_sample_s"] = 250;

  Config cfg;
  TEST_ASSERT_TRUE(config::fromJson(cfg, doc));
  TEST_ASSERT_EQUAL_UINT8(7, cfg.brightness_level);
  TEST_ASSERT_EQUAL_UINT16(1, cfg.page_switch_s);
  TEST_ASSERT_EQUAL_UINT8(24, cfg.hour_format);
  TEST_ASSERT_EQUAL_UINT8(60, cfg.env_sample_s);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_match_requirements);
  RUN_TEST(test_json_roundtrip);
  RUN_TEST(test_schema_mismatch_keeps_defaults);
  RUN_TEST(test_empty_doc_keeps_defaults);
  RUN_TEST(test_partial_doc_uses_defaults_for_missing);
  RUN_TEST(test_out_of_range_clamped);
  return UNITY_END();
}
