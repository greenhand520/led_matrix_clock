// LED 矩阵时钟 —— 应用入口（产品固件）
// 当前阶段 P6：WiFiManager SoftAP 配网 + SNTP 对时写 RTC + 断线重连 + 状态提示覆盖层
// 完整功能链：显示/时钟/温湿度/按键/亮度/动画 + 网络（P1..P6）
#include <Arduino.h>

#include "app/scheduler.h"
#include "config/config_store.h"
#include "display/display.h"
#include "display/tm1640.h"
#include "net/net_service.h"
#include "rtc/ds1302.h"
#include "sensor/dht.h"
#include "ui/anim_digit.h"
#include "ui/brightness.h"
#include "ui/button.h"
#include "ui/date_line.h"
#include "ui/overlay.h"
#include "ui/page_env.h"
#include "ui/page_time.h"
#include "ui/text_scroller.h"

namespace {

constexpr uint8_t kPinButton = 0;  // BOOT/用户按键（低有效，需求 R-KEY）
constexpr uint8_t kPageTime = 0;
constexpr uint8_t kPageEnv = 1;

Scheduler g_scheduler;
ConfigStore g_config;
Tm1640Chain g_bus;  // 显示后端：换芯片时替换此实现
Display g_display(g_bus);
FrameBuffer g_framebuffer;
Ds1302 g_rtc;
Dht g_dht;

struct AppCtx {
  Display* disp = nullptr;
  FrameBuffer* fb = nullptr;
  Ds1302* rtc = nullptr;
  Dht* dht = nullptr;
  ConfigStore* store = nullptr;
  const Config* cfg = nullptr;
  NetService net;
  StatusOverlay overlay;
  DateTime now = {};
  bool timeValid = false;
  uint8_t page = kPageTime;
  DigitAnimator anim;
  TextScroller dateScroll;
  AutoBrightness brightness;
  Button button;
  uint32_t lastPageSwitchMs = 0;
  uint32_t nextDateScrollMs = 0;
  uint32_t nextNtpWriteMs = 0;
  uint8_t lastBrightness = 0xFF;
  bool ntpWritten = false;
};

AppCtx g_app;

void switchPage(uint32_t nowMs) {
  g_app.page = (g_app.page == kPageTime) ? kPageEnv : kPageTime;
  g_app.lastPageSwitchMs = nowMs;
  Serial.printf("[ui] page -> %s\n", g_app.page == kPageTime ? "TIME" : "ENV");
}

// 整屏刷新任务（50ms）：动画步进 → 清屏 → 页面 → 日期滚动覆盖 → 状态覆盖 → 下发
void refreshTask(void*) {
  g_app.anim.step();
  if (g_app.dateScroll.running()) {
    g_app.dateScroll.step();
  }
  g_app.fb->clear();
  if (g_app.page == kPageTime) {
    if (g_app.timeValid) {
      page_time::render(*g_app.fb, g_app.now, *g_app.cfg, g_app.anim);
    }
  } else {
    const bool ok = g_app.dht->valid();
    page_env::render(*g_app.fb, g_app.dht->temperatureTenths(), g_app.dht->humidity(), ok, ok);
  }
  // 层级（布局稿 §5）：状态覆盖 > 日期滚动 > 页面内容
  if (g_app.dateScroll.running()) {
    g_app.dateScroll.draw(*g_app.fb, 0, Color::GREEN, FontKind::GLCD);
  }
  if (g_app.overlay.active(millis())) {
    g_app.overlay.draw(*g_app.fb);
  }
  g_app.disp->flush(*g_app.fb);
}

// 时钟推进任务（1s）：读 RTC → 更新显示时间（R-RTC-02）
void clockTickTask(void*) {
  DateTime t;
  if (g_app.rtc->readTime(t)) {
    g_app.now = t;
    g_app.timeValid = true;
  } else {
    Serial.println(F("[rtc] invalid time, keep last"));
  }
}

// 采样任务（cfg.env_sample_s，默认 3s）：启动一次 DHT 事务
void envSampleTask(void*) {
  if (!g_app.dht->startSample(millis())) {
    Serial.println(F("[dht] busy, skip sample"));
  }
}

// 页面轮换（cfg.page_switch_s，默认 15s）：TIME ↔ ENV；短按切换会重置计时（R-UI-08）
void pageRotateTask(void*) {
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - g_app.lastPageSwitchMs) >=
      static_cast<uint32_t>(g_app.cfg->page_switch_s) * 1000u) {
    switchPage(now);
  }
}

// 日期滚动触发（cfg.date_scroll_min，默认 30min，R-UI-04）
void dateScrollTask(void*) {
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - g_app.nextDateScrollMs) < 0) {
    return;
  }
  char text[32];
  buildDateText(g_app.now, g_app.cfg->date_order, text, sizeof(text));
  g_app.dateScroll.start(text);
  g_app.nextDateScrollMs = now + static_cast<uint32_t>(g_app.cfg->date_scroll_min) * 60000u;
  Serial.printf("[ui] date scroll: %s\n", text);
}

// 亮度任务（1s）：自动模式采样 A0（5516 光敏），手动档启动时已设置（R-BRI）
void brightnessTask(void*) {
  if (g_app.cfg->brightness_mode != 0) {
    return;
  }
  const uint8_t lv = g_app.brightness.update(analogRead(A0));
  if (lv != g_app.lastBrightness) {
    g_app.disp->setBrightness(lv);
    g_app.lastBrightness = lv;
  }
}

// 网络任务（1s）：配网推进/重连 + 状态提示 + SNTP → RTC（R-NET）
void netTask(void*) {
  static NetState prev = NetState::CONNECTING;
  const uint32_t now = millis();
  g_app.net.tick();

  if (g_app.net.consumeConnected()) {
    g_app.overlay.show("OK!!", now, false);  // 联网成功 2s
  }
  if (g_app.net.consumePortalDone()) {
    Serial.println(F("[net] portal done, restart into STA"));
    delay(200);
    ESP.restart();  // 凭据已保存，重启进 STA（R-NET-02）
  }
  if (g_app.net.consumePortalTimeout()) {
    g_app.overlay.show("RTC!", now, false);  // 配网超时 2s
  }
  if (g_app.net.state() == NetState::OFFLINE && prev != NetState::OFFLINE) {
    g_app.overlay.show("Err!", now, false);  // 联网失败 2s，之后按重试节奏再提示
  }
  prev = g_app.net.state();

  // SNTP → RTC：首次就绪即写；此后每 ntp_interval_h 后台校时（R-RTC-07）
  DateTime t;
  if (g_app.net.now(t)) {
    const uint32_t intervalMs = static_cast<uint32_t>(g_app.cfg->ntp_interval_h) * 3600000u;
    if (!g_app.ntpWritten || static_cast<int32_t>(now - g_app.nextNtpWriteMs) >= 0) {
      if (g_app.rtc->writeTime(t)) {
        g_app.ntpWritten = true;
        g_app.nextNtpWriteMs = now + intervalMs;
        Serial.println(F("[ntp] rtc updated from sntp"));
      }
    }
  }
}

// 按键任务（10ms）：消抖与长按分级（R-KEY-01）
void buttonTask(void*) {
  const bool pressed = digitalRead(kPinButton) == LOW;
  switch (g_app.button.poll(pressed, millis())) {
    case KeyEvt::SHORT:
      switchPage(millis());  // 立即换页并重置轮换计时
      break;
    case KeyEvt::LONG_2S:
      g_app.net.startPortal();  // SoftAP 配网（R-NET-02）
      g_app.overlay.show("Setup", millis(), true);
      Serial.println(F("[net] portal started by button"));
      break;
    case KeyEvt::LONG_5S:
      Serial.println(F("[cfg] factory reset"));
      g_app.store->factoryReset();
      delay(100);
      ESP.restart();
      break;
    default:
      break;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] LED matrix clock, P6 network"));

  // 配置加载：首次启动/损坏/版本不符时回退默认值（需求 R-CFG-01）
  if (g_config.begin()) {
    Serial.println(F("[config] loaded from flash"));
  } else {
    Serial.println(F("[config] use defaults"));
  }
  g_app.store = &g_config;
  g_app.cfg = &g_config.get();
  const Config& cfg = *g_app.cfg;
  Serial.printf("[config] tz=%d min, ntp=%s, page_switch=%u s, env_sample=%u s\n",
                cfg.tz_offset_min, cfg.ntp_server,
                static_cast<unsigned>(cfg.page_switch_s),
                static_cast<unsigned>(cfg.env_sample_s));

  // 显示：初始化 + 上电自检（R-UI-11）
  g_display.begin(cfg.brightness_level);
  g_display.selfTest(g_framebuffer);
  g_app.disp = &g_display;
  g_app.fb = &g_framebuffer;

  // 时钟：初始化并立即读一次（≤3s 显示时间，R-UI-10）
  g_app.rtc = &g_rtc;
  g_rtc.begin();
  DateTime t;
  if (g_rtc.readTime(t)) {
    g_app.now = t;
    g_app.timeValid = true;
    Serial.printf("[rtc] %04u-%02u-%02u %02u:%02u:%02u wday=%u\n", t.year, t.mon, t.day,
                  t.hour, t.min, t.sec, t.wday);
  } else {
    Serial.println(F("[rtc] no valid time yet"));
  }

  // 传感器 / 按键 / 起始页 / 亮度初值 / 时间轴
  g_app.dht = &g_dht;
  g_dht.begin();
  pinMode(kPinButton, INPUT_PULLUP);
  g_app.page = (cfg.default_page == PAGE_DEFAULT_ENV) ? kPageEnv : kPageTime;
  g_app.lastPageSwitchMs = millis();
  g_app.nextDateScrollMs = millis() + static_cast<uint32_t>(cfg.date_scroll_min) * 60000u;
  if (cfg.brightness_mode != 0) {
    g_display.setBrightness(cfg.brightness_level);
    g_app.lastBrightness = cfg.brightness_level;
  }

  // 网络：STA 自动连接 / 无凭据进配网；SNTP 后台同步（R-NET）
  g_app.net.begin(cfg.tz_offset_min, cfg.ntp_server);
  if (g_app.net.portalActive()) {
    g_app.overlay.show("Setup", millis(), true);
  } else {
    g_app.overlay.show("Wi-Fi", millis(), true);  // 连接中，持续到出结果
  }

  g_scheduler.add(50, refreshTask, nullptr);
  g_scheduler.add(1000, clockTickTask, nullptr);
  g_scheduler.add(static_cast<uint32_t>(cfg.env_sample_s) * 1000, envSampleTask, nullptr);
  g_scheduler.add(1000, pageRotateTask, nullptr);
  g_scheduler.add(1000, dateScrollTask, nullptr);
  g_scheduler.add(1000, brightnessTask, nullptr);
  g_scheduler.add(1000, netTask, nullptr);
  g_scheduler.add(10, buttonTask, nullptr);
  Serial.println(F("[display] init ok, all tasks armed"));
}

void loop() {
  // 所有周期任务经调度器截止时间驱动，禁止 delay()（设计文档 §2）
  g_scheduler.tick(millis());
  g_dht.tick(millis());  // 起始低电平到点后触发读取突发（18ms 状态机）
  yield();
}
