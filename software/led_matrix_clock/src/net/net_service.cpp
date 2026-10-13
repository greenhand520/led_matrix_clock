// 网络服务实现：WiFiManager（非阻塞配网）+ SNTP + 重连状态机
#include "net/net_service.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <time.h>

namespace {
WiFiManager g_wm;
constexpr char kPortalSsid[] = "MatrixClock-Setup";
constexpr uint32_t kPortalTimeoutMs = 180000;  // 配网超时（R-NET-02）
constexpr uint32_t kReconnectMs = 10000;       // 断线重连退避
constexpr time_t kMinValidEpoch = 1577836800;  // 2020-01-01，早于此视为未同步
}  // namespace

void NetService::begin(int16_t tzOffsetMin, const char* ntpServer) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  // SNTP 后台同步；time()/localtime_r 返回本地时区时间（R-NET-03/04）
  configTime(static_cast<long>(tzOffsetMin) * 60L, 0, ntpServer);

  if (!g_wm.getWiFiIsSaved()) {
    startPortalInternal();  // 无凭据 → 自动进入配网（R-NET-02）
    state_ = NetState::PORTAL;
  } else {
    WiFi.begin();  // 自动连接已保存凭据（R-NET-01）
    lastAttemptMs_ = millis();
    state_ = NetState::CONNECTING;
  }
}

void NetService::startPortalInternal() {
  g_wm.setConfigPortalBlocking(false);
  g_wm.setConfigPortalTimeout(kPortalTimeoutMs / 1000);
  g_wm.startConfigPortal(kPortalSsid);
  portalActive_ = true;
  portalDeadlineMs_ = millis() + kPortalTimeoutMs;
}

void NetService::startPortal() {
  if (!portalActive_) {
    startPortalInternal();
    state_ = NetState::PORTAL;
  }
}

bool NetService::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

void NetService::tick() {
  const uint32_t nowMs = millis();

  if (portalActive_) {
    g_wm.process();
    if (connected()) {
      portalActive_ = false;
      portalDone_ = true;  // 凭据已由 WiFiManager 保存 → 调用方重启进 STA
      state_ = NetState::CONNECTED;
      return;
    }
    if (static_cast<int32_t>(nowMs - portalDeadlineMs_) >= 0) {
      g_wm.stopConfigPortal();
      portalActive_ = false;
      portalTimeout_ = true;  // 调用方提示 RTC!，转入后台重连
      state_ = NetState::OFFLINE;
    }
    return;
  }

  if (connected()) {
    if (state_ != NetState::CONNECTED) {
      state_ = NetState::CONNECTED;
      connectedEdge_ = true;
    }
    return;
  }

  // 断线：退避重连，显示走时不受影响（R-NET-05）
  if (state_ == NetState::CONNECTED) {
    state_ = NetState::OFFLINE;
  }
  if (static_cast<uint32_t>(nowMs - lastAttemptMs_) >= kReconnectMs) {
    lastAttemptMs_ = nowMs;
    WiFi.reconnect();
    state_ = NetState::CONNECTING;
  }
}

bool NetService::now(DateTime& out) const {
  const time_t t = time(nullptr);
  if (t < kMinValidEpoch) {
    return false;
  }
  struct tm tmv;
  localtime_r(&t, &tmv);
  DateTime dt;
  dt.year = static_cast<uint16_t>(tmv.tm_year + 1900);
  dt.mon = static_cast<uint8_t>(tmv.tm_mon + 1);
  dt.day = static_cast<uint8_t>(tmv.tm_mday);
  dt.hour = static_cast<uint8_t>(tmv.tm_hour);
  dt.min = static_cast<uint8_t>(tmv.tm_min);
  dt.sec = static_cast<uint8_t>(tmv.tm_sec);
  dt.wday = static_cast<uint8_t>(tmv.tm_wday + 1);  // 1=周日..7=周六（date_line 约定）
  if (!validDateTime(dt)) {
    return false;
  }
  out = dt;
  return true;
}

bool NetService::consumeConnected() {
  const bool v = connectedEdge_;
  connectedEdge_ = false;
  return v;
}

bool NetService::consumePortalDone() {
  const bool v = portalDone_;
  portalDone_ = false;
  return v;
}

bool NetService::consumePortalTimeout() {
  const bool v = portalTimeout_;
  portalTimeout_ = false;
  return v;
}
