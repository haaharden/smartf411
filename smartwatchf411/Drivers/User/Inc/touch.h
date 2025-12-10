#ifndef _TOUCH_H
#define _TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"
	
extern volatile uint8_t touch_int_flag;

//MAX的I2C从机地址
#define CST816_ADDRESS                0x2a    
	
/* 触摸屏寄存器 */
#define GestureID     0x01 // 手势寄存器
#define FingerNum     0x02 // 手指数量
#define XposH         0x03 // x高四位
#define XposL         0x04 // x低八位
#define YposH         0x05 // y高四位
#define YposL         0x06 // y低八位
#define ChipID        0xA7 // 芯片型号
#define MotionMask    0xEC // 触发动作
#define AutoSleepTime 0xF9 // 自动休眠
#define IrqCrl        0xFA // 中断控制
#define AutoReset     0xFB // 无手势休眠
#define LongPressTime 0xFC // 长按休眠
#define DisAutoSleep  0xFE // 使能低功耗模式

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

typedef struct TouchPoint {
    uint8_t Gesture;
    uint8_t Finger;
    uint16_t X;
    uint16_t Y;
} TouchType;  

typedef enum {
    EVENT_NONE = 0,

    EVENT_SINGLE_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS,

    EVENT_SLIDE_UP,//拉出通知栏
    EVENT_SLIDE_DOWN,//拉出快捷开关等
    EVENT_SLIDE_LEFT,//切到下一页表盘
    EVENT_SLIDE_RIGHT,//返回上一页
} TouchEvent;


void CST816T_Init(void);
uint8_t CST816_GetAction(uint16_t *X, uint16_t *Y,uint8_t *Gesture, uint8_t *pFingerNum);
TouchEvent GetTouchEvent(uint16_t x, uint16_t y, uint8_t gesture, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif
