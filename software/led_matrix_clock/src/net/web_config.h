#pragma once
// Web 配置服务（P7，需求 R-WEB）：STA 连接后常驻，浏览器访问设备 IP 编辑全部配置项
// 与 WiFiManager 配网页共用 80 端口：配网进行中由调用方停止本服务（互斥占用）
#include <ESP8266WebServer.h>

#include "config/config_store.h"

class WebConfig {
 public:
  void begin(ConfigStore& store);  // 注册路由并监听 80
  void stop();                     // 停止监听（让位配网页）
  void tick();                     // 高频调用：handleClient

  bool active() const { return active_; }

 private:
  void handleRoot();
  void handleSave();
  void handleReset();
  void sendPage(const char* notice);

  ESP8266WebServer server_{80};
  ConfigStore* store_ = nullptr;
  bool active_ = false;
};
