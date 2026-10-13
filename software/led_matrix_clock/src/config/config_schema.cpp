// 配置 JSON 序列化/反序列化实现（纯逻辑，可主机侧单测）
#include "config/config_schema.h"

#include <stdio.h>
#include <string.h>

namespace {

// 安全字符串拷贝，保证 NUL 结尾
void copyStr(char* dst, size_t dstSize, const char* src) {
  snprintf(dst, dstSize, "%s", src);
}

template <typename T>
T clampVal(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

}  // namespace

namespace config {

void toJson(const Config& cfg, JsonDocument& doc) {
  doc.clear();
  doc["schema"] = kSchema;
  doc["tz_offset_min"] = cfg.tz_offset_min;
  doc["ntp_server"] = cfg.ntp_server;
  doc["ntp_interval_h"] = cfg.ntp_interval_h;
  doc["hour_format"] = cfg.hour_format;
  doc["date_order"] = cfg.date_order;
  doc["rotate_mode"] = cfg.rotate_mode;
  doc["colon_blink"] = cfg.colon_blink;
  doc["brightness_mode"] = cfg.brightness_mode;
  doc["brightness_level"] = cfg.brightness_level;
  doc["default_page"] = cfg.default_page;
  doc["page_switch_s"] = cfg.page_switch_s;
  doc["env_sample_s"] = cfg.env_sample_s;
  doc["date_scroll_min"] = cfg.date_scroll_min;
}

bool fromJson(Config& cfg, const JsonDocument& doc) {
  if (doc.size() == 0) {
    return false;  // 空文档：视为无配置
  }
  if (static_cast<uint32_t>(doc["schema"] | 0) != kSchema) {
    return false;  // 版本不符：整份回退默认值
  }

  // 缺字段沿用当前值（通常为默认值），越界值收敛到合法范围
  cfg.tz_offset_min =
      clampVal<int16_t>(doc["tz_offset_min"] | cfg.tz_offset_min, -720, 840);
  const char* ntp = doc["ntp_server"] | static_cast<const char*>(nullptr);
  if (ntp != nullptr) {
    copyStr(cfg.ntp_server, sizeof(cfg.ntp_server), ntp);
  }
  cfg.ntp_interval_h =
      clampVal<uint8_t>(doc["ntp_interval_h"] | cfg.ntp_interval_h, 1, 24);
  cfg.hour_format = ((doc["hour_format"] | cfg.hour_format) == 12) ? 12 : 24;
  cfg.date_order = clampVal<uint8_t>(doc["date_order"] | cfg.date_order, 0, 3);
  cfg.rotate_mode = clampVal<uint8_t>(doc["rotate_mode"] | cfg.rotate_mode, 0, 3);
  cfg.colon_blink = (doc["colon_blink"] | cfg.colon_blink) ? 1 : 0;
  cfg.brightness_mode = (doc["brightness_mode"] | cfg.brightness_mode) ? 1 : 0;
  cfg.brightness_level =
      clampVal<uint8_t>(doc["brightness_level"] | cfg.brightness_level, 0, 7);
  cfg.default_page = (doc["default_page"] | cfg.default_page) ? 1 : 0;
  cfg.page_switch_s =
      clampVal<uint16_t>(doc["page_switch_s"] | cfg.page_switch_s, 1, 3600);
  cfg.env_sample_s =
      clampVal<uint8_t>(doc["env_sample_s"] | cfg.env_sample_s, 1, 60);
  cfg.date_scroll_min = clampVal<uint16_t>(
      doc["date_scroll_min"] | cfg.date_scroll_min, 1, 1440);
  return true;
}

}  // namespace config
