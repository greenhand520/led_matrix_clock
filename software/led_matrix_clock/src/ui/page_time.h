#pragma once
// PAGE_TIME 渲染（布局稿 v0.4 §2）：
// 大 4x7 时/分（红）+ 冒号（红，1Hz 闪可配）+ 小 3x5 秒（绿）+ 12h 角标（AM 绿/PM 橙）
#include "config/config_schema.h"
#include "display/framebuffer.h"
#include "rtc/datetime.h"

namespace page_time {

// 将时间绘制到帧缓冲（不含清屏）；12h 模式小时前导 0 不点亮
void render(FrameBuffer& fb, const DateTime& t, const Config& cfg);

}  // namespace page_time
