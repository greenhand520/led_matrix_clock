#pragma once
// 配置数据结构与 JSON 序列化（纯逻辑，无文件/硬件依赖，可主机侧单测）
// 字段与默认值对应需求文档 §8 配置项清单；WiFi 凭据由 WiFiManager 独立管理

#include <ArduinoJson.h>
#include <stdint.h>

// 显示方向枚举定义于 display/framebuffer.h（RotateMode）；
// 配置仅持久化其数值：0=正常 1=旋转180° 2=水平镜像 3=垂直镜像

// 上电默认显示页（需求 R-UI-12）
enum DefaultPage : uint8_t {
  PAGE_DEFAULT_TIME = 0,
  PAGE_DEFAULT_ENV = 1,
};

struct Config {
  int16_t tz_offset_min = 480;          // 时区偏移（分钟），默认 UTC+8
  char ntp_server[32] = "time1.aliyun.com";
  uint8_t ntp_interval_h = 6;           // 后台对时间隔（小时）
  uint8_t hour_format = 24;             // 12 / 24
  uint8_t date_order = 0;               // 日期顺序：0 = YYYY MM DD 星期
  uint8_t rotate_mode = 0;              // RotateMode 数值（0..3）
  uint8_t colon_blink = 1;              // 冒号 1Hz 闪烁开关（0 = 常亮）
  uint8_t brightness_mode = 0;          // 0 = 自动（5516 光敏）/ 1 = 手动
  uint8_t brightness_level = 5;         // 手动档位 0..7
  uint8_t default_page = PAGE_DEFAULT_TIME;
  uint16_t page_switch_s = 15;          // 页面轮换周期（秒）
  uint8_t env_sample_s = 3;             // DHT11 采样周期（秒）
  uint16_t date_scroll_min = 30;        // 日期滚动周期（分钟）
};

namespace config {

// 配置结构版本：与文件中 schema 字段不符时整份回退默认值
constexpr uint32_t kSchema = 1;

// Config -> JsonDocument
void toJson(const Config& cfg, JsonDocument& doc);

// JsonDocument -> Config；空文档或 schema 不符返回 false（cfg 保持原值）
// 缺字段沿用默认值，越界值收敛到合法范围
bool fromJson(Config& cfg, const JsonDocument& doc);

}  // namespace config
