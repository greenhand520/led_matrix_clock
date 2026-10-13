#pragma once
// 配置持久化：LittleFS /config.json
// 加载失败（首次启动/文件损坏/版本不符）一律回退默认值，不中断启动

#include "config/config_schema.h"

class ConfigStore {
 public:
  // 挂载文件系统并加载配置；成功从文件加载返回 true，回退默认值返回 false
  bool begin();

  // 写回 /config.json
  bool save();

  // 恢复出厂：删除配置文件并回到默认值（需求 R-CFG-03）
  void factoryReset();

  Config& get() { return cfg_; }
  const Config& get() const { return cfg_; }

 private:
  Config cfg_;
  bool fsReady_ = false;
};
