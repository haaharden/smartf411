#ifndef __BACKLIGHT_H
#define __BACKLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include "adc.h"
#include <stdint.h>

/* 背光模块 API */

/* 初始化背光：启动 PWM，设默认亮度 */
void Back_Init(void);

/* 手动设置背光亮度（0~100%） */
void Back_SetPercent(uint8_t percent);

/* 读取当前背光亮度百分比（0~100） */
uint8_t Back_GetPercent(void);

/* 自动亮度开关 */
void Back_SetAuto(uint8_t enable);
uint8_t Back_GetAuto(void);

/* 主循环/GUI 任务中周期调用，根据光敏自动调整亮度 */
void Back_UpdateAuto(void);

/* 读取最近一次的光敏原始值（ADC） */
uint16_t Back_GetAmbientRaw(void);

#ifdef __cplusplus
}
#endif

#endif /* __BACKLIGHT_H */
