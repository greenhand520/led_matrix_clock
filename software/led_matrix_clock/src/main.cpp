// LED 矩阵时钟 —— 应用入口（产品固件）
// 当前阶段 P4：TIME↔ENV 每 page_switch_s 轮换；DHT11 采样（env_sample_s）；
// PAGE_ENV 舒适度着色；网络任务在 P6 接入
#include <Arduino.h>

#include "app/scheduler.h"
#include "config/config_store.h"
#include "display/display.h"
#include "display/tm1640.h"
#include "rtc/ds1302.h"
#include "sensor/dht.h"
#include "ui/page_env.h"
#include "ui/page_time.h"

namespace {

Scheduler g_scheduler;
ConfigStore g_config;
Tm1640Chain g_bus;  // 显示后端：换芯片时替换此实现
Display g_display(g_bus);
FrameBuffer g_framebuffer;
Ds1302 g_rtc;
Dht g_dht;

constexpr uint8_t kPageTime = 0;
constexpr uint8_t kPageEnv = 1;

struct AppCtx {
  Display* disp;
  FrameBuffer* fb;
  Ds1302* rtc;
  Dht* dht;
  const Config* cfg;
  DateTime now;
  bool timeValid;
  uint8_t page;
};

AppCtx g_app = {&g_display, &g_framebuffer, &g_rtc, &g_dht, nullptr, {}, false, kPageTime};

// 整屏刷新任务（50ms）：清屏 → 渲染当前页 → 下发
void refreshTask(void* ctx) {
  AppCtx* app = static_cast<AppCtx*>(ctx);
  app->fb->clear();
  if (app->page == kPageTime) {
    if (app->timeValid) {
      page_time::render(*app->fb, app->now, *app->cfg);
    }
  } else {
    // 温湿度同组有效性由传感器校验决定；失败显示 "--"（R-ENV-03）
    const bool ok = app->dht->valid();
    page_env::render(*app->fb, app->dht->temperatureTenths(), app->dht->humidity(), ok, ok);
  }
  app->disp->flush(*app->fb);
}

// 时钟推进任务（1s）：读 RTC → 更新显示时间（R-RTC-02）
void clockTickTask(void* ctx) {
  AppCtx* app = static_cast<AppCtx*>(ctx);
  DateTime t;
  if (app->rtc->readTime(t)) {
    app->now = t;
    app->timeValid = true;
  } else {
    Serial.println(F("[rtc] invalid time, keep last"));
  }
}

// 采样任务（cfg.env_sample_s，默认 3s）：启动一次 DHT11 事务
void envSampleTask(void* ctx) {
  AppCtx* app = static_cast<AppCtx*>(ctx);
  if (!app->dht->startSample(millis())) {
    Serial.println(F("[dht] busy, skip sample"));
  }
}

// 页面轮换（cfg.page_switch_s，默认 15s）：TIME ↔ ENV（R-UI-08）
void pageSwitchTask(void* ctx) {
  AppCtx* app = static_cast<AppCtx*>(ctx);
  app->page = (app->page == kPageTime) ? kPageEnv : kPageTime;
  Serial.printf("[ui] page -> %s\n", app->page == kPageTime ? "TIME" : "ENV");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] LED matrix clock, P4 env page"));

  // 配置加载：首次启动/损坏/版本不符时回退默认值（需求 R-CFG-01）
  if (g_config.begin()) {
    Serial.println(F("[config] loaded from flash"));
  } else {
    Serial.println(F("[config] use defaults"));
  }
  g_app.cfg = &g_config.get();
  const Config& cfg = *g_app.cfg;
  Serial.printf("[config] tz=%d min, ntp=%s, page_switch=%u s, env_sample=%u s\n",
                cfg.tz_offset_min, cfg.ntp_server,
                static_cast<unsigned>(cfg.page_switch_s),
                static_cast<unsigned>(cfg.env_sample_s));

  // 显示：初始化 + 上电自检（R-UI-11）
  g_display.begin(cfg.brightness_level);
  g_display.selfTest(g_framebuffer);

  // 时钟：初始化并立即读一次（≤3s 显示时间，R-UI-10）
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

  // 传感器与起始页（default_page）
  g_dht.begin();
  g_app.page = (cfg.default_page == PAGE_DEFAULT_ENV) ? kPageEnv : kPageTime;

  g_scheduler.add(50, refreshTask, &g_app);
  g_scheduler.add(1000, clockTickTask, &g_app);
  g_scheduler.add(static_cast<uint32_t>(cfg.env_sample_s) * 1000, envSampleTask, &g_app);
  g_scheduler.add(static_cast<uint32_t>(cfg.page_switch_s) * 1000, pageSwitchTask, &g_app);
  Serial.println(F("[display] init ok, tasks armed"));
}

void loop() {
  // 所有周期任务经调度器截止时间驱动，禁止 delay()（设计文档 §2）
  g_scheduler.tick(millis());
  g_dht.tick(millis());  // 起始低电平到点后触发读取突发（18ms 状态机）
  yield();
}
