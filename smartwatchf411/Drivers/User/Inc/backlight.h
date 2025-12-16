#ifndef __BACKLIGHT_H
#define __BACKLIGHT_H

#include <stdint.h>

void Back_Init(void);                // 初始化
void Back_SetBrightness(uint8_t pct); // 设置亮度 (0~100)

#endif
