// LED 矩阵时钟 —— 应用入口
// 当前阶段 P1：工程骨架 + 协作式调度框架 + 配置服务；
// 显示/时钟/传感器/网络任务在后续阶段（P2 起）注册进调度器
#include <Arduino.h>

#include "app/scheduler.h"
#include "config/config_store.h"

Scheduler g_scheduler;
ConfigStore g_config;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[boot] LED matrix clock, P1 skeleton"));

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
}

void loop() {
  // 所有周期任务经调度器截止时间驱动，禁止 delay()（设计文档 §2）
  g_scheduler.tick(millis());
  yield();
}
