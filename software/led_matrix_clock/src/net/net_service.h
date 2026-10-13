#pragma once
// 网络服务（P6）：WiFiManager SoftAP 配网 + STA 自动连接/断线重连 + SNTP 时间源
// 约束（硬件设计）：禁用 Serial1；配网 AP = MatrixClock-Setup，180s 超时（R-NET-02）
// 时区按 tzOffsetMin 注入 configTime（不做夏令时，R-NET-04）；时间即本地时区
#include <stdint.h>

#include "rtc/datetime.h"

enum class NetState : uint8_t { CONNECTING, CONNECTED, PORTAL, OFFLINE };

class NetService {
 public:
  // tzOffsetMin：时区偏移（分钟）；ntpServer：NTP 服务器域名
  void begin(int16_t tzOffsetMin, const char* ntpServer);

  // 1s 节拍：配网过程推进 / 断线退避重连（R-NET-05）
  void tick();

  // 长按 2s 触发 SoftAP 配网（已在配网则忽略）
  void startPortal();

  NetState state() const { return state_; }
  bool connected() const;
  bool portalActive() const { return portalActive_; }

  // SNTP 时间（本地时区）就绪则输出（调用方写 RTC）；未同步返回 false
  bool now(DateTime& out) const;

  // 事件标志（读后清除）
  bool consumeConnected();       // 联网成功沿 → 提示 OK!!
  bool consumePortalDone();      // 配网成功（凭据已存）→ 重启进 STA
  bool consumePortalTimeout();   // 配网超时 → 提示 RTC!

 private:
  void startPortalInternal();

  NetState state_ = NetState::CONNECTING;
  bool portalActive_ = false;
  bool connectedEdge_ = false;
  bool portalDone_ = false;
  bool portalTimeout_ = false;
  uint32_t lastAttemptMs_ = 0;
  uint32_t portalDeadlineMs_ = 0;
};
