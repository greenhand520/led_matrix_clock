#pragma once
// Adafruit GFX 经典 glcdfont（5x7：5 列/字符，列字节 LSB = 最上行）
// 来源：Adafruit-GFX-Library glcdfont.c（BSD-3，全文收录于 glcdfont.c 头部，字模数据未改）

#include <stdint.h>

extern "C" {
extern const unsigned char glcdfont[];  // 0x20-0x7F，每字符 5 字节
}
