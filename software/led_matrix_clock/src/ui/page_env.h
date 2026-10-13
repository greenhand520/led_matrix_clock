#pragma once
// PAGE_ENV 渲染（布局稿 v0.4 §3）：全 3x5，7 槽位 27 列
// 左组温度 `25.3°C`（0.1℃）+ 小数点落间隙列；右组湿度 `45%`；各按舒适度着色
#include <stdint.h>

#include "display/framebuffer.h"

namespace page_env {

// tempTenths：0.1℃（253 = 25.3℃，负值用 '-' 槽）；humPercent：%
// tempValid/humValid 为假时对应组显示 "--"（R-ENV-03，红色）
void render(FrameBuffer& fb, int16_t tempTenths, uint8_t humPercent,
            bool tempValid, bool humValid);

}  // namespace page_env
