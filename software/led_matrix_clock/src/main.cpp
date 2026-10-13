// LED 矩阵时钟 —— 应用入口（产品固件）
// 当前阶段 P3：PAGE_TIME 时间页接入（DS1302 走时 + 大 4x7 时分 + 冒号 + 小 3x5 秒 + 12h 角标）
// 温湿度页、传感器、网络任务在后续阶段（P4 起）注册进调度器
#include <Arduino.h>

#include "app/scheduler.h"
#include "config/config_store.h"
#include "display/display.h"
#include "display/tm1640.h"
#include "rtc/ds1302.h"
#include "ui/page_time.h"

namespace {

Scheduler g_scheduler;
ConfigStore g_config;
Tm1640Chain g_bus;  // 显示后端：换芯片时替换此实现
Display g_display(g_bus);
FrameBuffer g_framebuffer;
Ds1302 g_rtc;

struct AppCtx {
  Display* disp;
  FrameBuffer* fb;
  Ds1302* rtc;
  const Config* cfg;
  DateTime now;
  bool timeValid;
};

AppCtx g_app = {&g_display, &g_framebuffer, &g_rtc, nullptr, {}, false};

// 整屏刷新任务（50ms，设计文档 §2 节拍表）：清屏 → 渲染当前页 → 下发
void refreshTask(void* ctx) {
  AppCtx* app = static_cast<AppCtx*>(ctx);
  app->fb->clear();
  if (app->timeValid) {
    page_time::render(*app->fb, app->now, *app->cfg);
  }
  app->disp->flush(*app->fb);
}

// 时钟推进任务（1s）：读 RTC → 更新显示时间（R-RTC-02）；越界拒绝并保持上次值（R-RTC-06）
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

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] LED matrix clock, P3 time page"));

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

  g_scheduler.add(50, refreshTask, &g_app);
  g_scheduler.add(1000, clockTickTask, &g_app);
  Serial.println(F("[display] init ok, refresh + clock tasks armed"));
}

void loop() {
  // 所有周期任务经调度器截止时间驱动，禁止 delay()（设计文档 §2）
  g_scheduler.tick(millis());
  yield();
}
