// Web 配置服务实现：单页表单（13 项）+ 保存 + 恢复出厂
#include "net/web_config.h"

#include <Arduino.h>

namespace {

const char kHead[] =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>矩阵时钟配置</title><style>"
    "body{font-family:sans-serif;margin:1em;max-width:24em}"
    "label{display:block;margin:.6em 0 .15em;font-size:.9em}"
    "input,select{width:100%;padding:.35em}"
    "button{margin-top:1em;padding:.5em 1.2em;width:100%}"
    "</style></head><body><h2>矩阵时钟配置</h2>";

const char kTail[] = "</body></html>";

String option(int value, int current, const char* label) {
  String s("<option value=\"");
  s += value;
  s += (value == current) ? "\" selected>" : "\">";
  s += label;
  s += "</option>";
  return s;
}

}  // namespace

void WebConfig::begin(ConfigStore& store) {
  store_ = &store;
  server_.on("/", HTTP_GET, [this]() { sendPage(nullptr); });
  server_.on("/save", HTTP_POST, [this]() { handleSave(); });
  server_.on("/reset", HTTP_POST, [this]() { handleReset(); });
  server_.begin();
  active_ = true;
}

void WebConfig::stop() {
  server_.stop();
  active_ = false;
}

void WebConfig::tick() {
  if (active_) {
    server_.handleClient();
  }
}

void WebConfig::sendPage(const char* notice) {
  const Config& c = store_->get();
  String page(kHead);
  if (notice != nullptr) {
    page += "<p><b>";
    page += notice;
    page += "</b></p>";
  }
  page += "<form method=\"POST\" action=\"/save\">";

  page += "<label>时区偏移（分钟，480=UTC+8）</label><input name=\"tz_offset_min\" type=\"number\" value=\"";
  page += String(c.tz_offset_min);
  page += "\">";

  page += "<label>NTP 服务器</label><input name=\"ntp_server\" value=\"";
  page += c.ntp_server;
  page += "\">";

  page += "<label>后台对时间隔（小时）</label><input name=\"ntp_interval_h\" type=\"number\" min=\"1\" max=\"24\" value=\"";
  page += String(c.ntp_interval_h);
  page += "\">";

  page += "<label>小时制</label><select name=\"hour_format\">";
  page += option(24, c.hour_format, "24 小时");
  page += option(12, c.hour_format, "12 小时");
  page += "</select>";

  page += "<label>日期顺序</label><select name=\"date_order\">";
  page += option(0, c.date_order, "YYYY-MM-DD 星期");
  page += option(1, c.date_order, "DD-MM-YYYY 星期");
  page += option(2, c.date_order, "MM-DD-YYYY 星期");
  page += "</select>";

  page += "<label>显示方向</label><select name=\"rotate_mode\">";
  page += option(0, c.rotate_mode, "正常");
  page += option(1, c.rotate_mode, "旋转 180°");
  page += option(2, c.rotate_mode, "水平镜像");
  page += option(3, c.rotate_mode, "垂直镜像");
  page += "</select>";

  page += "<label><input name=\"colon_blink\" type=\"checkbox\" style=\"width:auto\"";
  page += (c.colon_blink ? " checked" : "");
  page += "> 冒号闪烁</label>";

  page += "<label>亮度模式</label><select name=\"brightness_mode\">";
  page += option(0, c.brightness_mode, "自动（光敏）");
  page += option(1, c.brightness_mode, "手动");
  page += "</select>";

  page += "<label>手动亮度（0–7）</label><input name=\"brightness_level\" type=\"number\" min=\"0\" max=\"7\" value=\"";
  page += String(c.brightness_level);
  page += "\">";

  page += "<label>默认显示页</label><select name=\"default_page\">";
  page += option(0, c.default_page, "时间");
  page += option(1, c.default_page, "温湿度");
  page += "</select>";

  page += "<label>页面轮换周期（秒）</label><input name=\"page_switch_s\" type=\"number\" min=\"1\" max=\"3600\" value=\"";
  page += String(c.page_switch_s);
  page += "\">";

  page += "<label>温湿度采样周期（秒）</label><input name=\"env_sample_s\" type=\"number\" min=\"1\" max=\"60\" value=\"";
  page += String(c.env_sample_s);
  page += "\">";

  page += "<label>日期滚动周期（分钟）</label><input name=\"date_scroll_min\" type=\"number\" min=\"1\" max=\"1440\" value=\"";
  page += String(c.date_scroll_min);
  page += "\">";

  page += "<button type=\"submit\">保存</button></form>";

  page += "<form method=\"POST\" action=\"/reset\" onsubmit=\"return confirm('恢复出厂并重启？')\">"
          "<button type=\"submit\">恢复出厂</button></form>";
  page += kTail;
  server_.send(200, "text/html; charset=utf-8", page);
}

void WebConfig::handleSave() {
  // 现值转 JSON → 请求参数覆盖 → 统一经 fromJson 校验/越界收敛（复用 R-CFG 校验）
  JsonDocument doc;
  config::toJson(store_->get(), doc);
  if (server_.hasArg("tz_offset_min")) doc["tz_offset_min"] = server_.arg("tz_offset_min").toInt();
  if (server_.hasArg("ntp_server")) doc["ntp_server"] = server_.arg("ntp_server");
  if (server_.hasArg("ntp_interval_h")) doc["ntp_interval_h"] = server_.arg("ntp_interval_h").toInt();
  if (server_.hasArg("hour_format")) doc["hour_format"] = server_.arg("hour_format").toInt();
  if (server_.hasArg("date_order")) doc["date_order"] = server_.arg("date_order").toInt();
  if (server_.hasArg("rotate_mode")) doc["rotate_mode"] = server_.arg("rotate_mode").toInt();
  doc["colon_blink"] = server_.hasArg("colon_blink") ? 1 : 0;
  if (server_.hasArg("brightness_mode")) doc["brightness_mode"] = server_.arg("brightness_mode").toInt();
  if (server_.hasArg("brightness_level")) doc["brightness_level"] = server_.arg("brightness_level").toInt();
  if (server_.hasArg("default_page")) doc["default_page"] = server_.arg("default_page").toInt();
  if (server_.hasArg("page_switch_s")) doc["page_switch_s"] = server_.arg("page_switch_s").toInt();
  if (server_.hasArg("env_sample_s")) doc["env_sample_s"] = server_.arg("env_sample_s").toInt();
  if (server_.hasArg("date_scroll_min")) doc["date_scroll_min"] = server_.arg("date_scroll_min").toInt();

  Config cfg = store_->get();
  if (config::fromJson(cfg, doc)) {
    store_->get() = cfg;
    if (store_->save()) {
      sendPage("已保存。部分项实时生效，时区/NTP 服务器/采样周期重启后生效。");
      return;
    }
  }
  sendPage("保存失败（校验未通过或写 flash 出错），已保持原值。");
}

void WebConfig::handleReset() {
  store_->factoryReset();
  server_.send(200, "text/html; charset=utf-8",
               "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>"
               "<p><b>已恢复出厂，设备重启中…</b></p></body></html>");
  delay(200);
  ESP.restart();
}
