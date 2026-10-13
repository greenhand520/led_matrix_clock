// LED 矩阵时钟 —— 应用入口（产品固件）
// 当前阶段 P2：显示子系统接入（初始化 + 上电自检 + 50ms 刷新任务）；
// 时间页/温湿度页、时钟、传感器、网络任务在后续阶段（P3 起）注册进调度器
#include <Arduino.h>

#include "app/scheduler.h"
#include "config/config_store.h"
#include "display/display.h"

namespace {

Scheduler g_scheduler;
ConfigStore g_config;
Display g_display;
FrameBuffer g_framebuffer;

struct RefreshCtx {
  Display* disp;
  FrameBuffer* fb;
};

// 整屏刷新任务（50ms，设计文档 §2 节拍表）
void refreshTask(void* ctx) {
  RefreshCtx* c = static_cast<RefreshCtx*>(ctx);
  c->disp->flush(*c->fb);
}

RefreshCtx g_refreshCtx = {&g_display, &g_framebuffer};

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] LED matrix clock, P2 display"));

  // 配置加载：首次启动/损坏/版本不符时回退默认值（需求 R-CFG-01）
  if (g_config.begin()) {
    Serial.println(F("[config] loaded from flash"));
  } else {
    Serial.println(F("[config] use defaults"));
  }
  const Config& cfg = g_config.get();
  Serial.printf("[config] tz=%d min, ntp=%s, page_switch=%u s, env_sample=%u s\n",
                cfg.tz_offset_min, cfg.ntp_server,
                static_cast<unsigned>(cfg.page_switch_s),
                static_cast<unsigned>(cfg.env_sample_s));

  // 显示：初始化 + 上电自检（R-UI-11）；亮度取配置初值（自动亮度 P5 接入）
  g_display.begin(cfg.brightness_level);
  g_display.selfTest(g_framebuffer);

  g_scheduler.add(50, refreshTask, &g_refreshCtx);
  Serial.println(F("[display] init ok, refresh task armed"));
}

void loop() {
  // 所有周期任务经调度器截止时间驱动，禁止 delay()（设计文档 §2）
  g_scheduler.tick(millis());
  yield();
}
