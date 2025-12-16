#include "backlight.h"
#include "tim.h"   // 确保这里面包含了 extern TIM_HandleTypeDef htim2;

/* 修改宏定义：现在的定时器是 TIM2，周期是 999 */
#define BACK_PWM_TIM       &htim2         // <--- 改成 TIM2
#define BACK_PWM_CHANNEL   TIM_CHANNEL_1
#define PWM_PERIOD_MAX     999            // <--- 改成 999

void Back_Init(void)
{
    // 1. 启动 TIM2 的 PWM
    HAL_TIM_PWM_Start(BACK_PWM_TIM, BACK_PWM_CHANNEL);

    // 2. 默认亮度
    Back_SetBrightness(100);
}

void Back_SetBrightness(uint8_t pct)
{
    if(pct > 100) pct = 100;
		if(pct < 10) pct = 10;//防止完全灭掉
    // 计算 CCR (0 ~ 999)
    // 比如 50% -> 50 * 999 / 100 = 499
    uint32_t ccr = (uint32_t)(pct * PWM_PERIOD_MAX / 100);

    __HAL_TIM_SET_COMPARE(BACK_PWM_TIM, BACK_PWM_CHANNEL, ccr);
}
