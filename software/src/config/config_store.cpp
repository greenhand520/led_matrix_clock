// 配置持久化实现：LittleFS /config.json
#include "config/config_store.h"

#include <LittleFS.h>

namespace {
constexpr char kPath[] = "/config.json";
}

bool ConfigStore::begin() {
  fsReady_ = LittleFS.begin();
  if (!fsReady_) {
    // 首次上电文件系统未初始化：格式化后重新挂载
    fsReady_ = LittleFS.format() && LittleFS.begin();
  }
  if (!fsReady_) {
    return false;  // 文件系统不可用：保持默认配置，不阻塞启动
  }

  File f = LittleFS.open(kPath, "r");
  if (!f) {
    return false;  // 首次启动无配置文件：用默认值
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    return false;  // JSON 损坏：用默认值
  }
  return config::fromJson(cfg_, doc);  // schema 不符同样回退默认值
}

bool ConfigStore::save() {
  if (!fsReady_) {
    return false;
  }
  JsonDocument doc;
  config::toJson(cfg_, doc);
  File f = LittleFS.open(kPath, "w");
  if (!f) {
    return false;
  }
  size_t n = serializeJson(doc, f);
  f.close();
  return n > 0;
}

void ConfigStore::factoryReset() {
  if (fsReady_) {
    LittleFS.remove(kPath);
  }
  cfg_ = Config();
}
