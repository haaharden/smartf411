#include "backlight.h"
#include "cmsis_os.h"

/* ======= 1. 绑定具体硬件资源：定时器 + ADC ======= */

/* 背光 PWM 使用的定时器和通道
 * 假设：你在 CubeMX 里把 TIM3 CH1 配成 PWM，输出到接 MOS 栅极的那个引脚
 * 如果用的是别的定时器或通道，改这里就行。
 */
extern TIM_HandleTypeDef htim3;
#define BACK_TIM            (&htim3)
#define BACK_TIM_CHANNEL    TIM_CHANNEL_1

/* 光敏传感器使用的 ADC
 * 假设：光敏接在 ADC1 某个通道，CubeMX 已配置好，句柄名 hadc1
 */
extern ADC_HandleTypeDef hadc1;
#define BACK_AMBIENT_ADC    (&hadc1)

/* ======= 2. 内部状态变量 ======= */

static uint8_t  s_back_auto      = 1;        /* 1=自动亮度；0=手动 */
static uint8_t  s_back_percent   = 50;       /* 当前亮度百分比 */
static uint32_t s_pwm_max        = 999;      /* PWM 计数上限（ARR） */

static uint16_t s_ambient_raw    = 0;        /* 最近一次光敏 ADC 原始值 */
static float    s_ambient_lp     = 0.0f;     /* 平滑后的光强（低通滤波） */

/* 你可以根据实际测试调整这两个阈值：
 * ADC_MIN: 最暗环境的 ADC 值（例如用手完全遮住光敏时观测）
 * ADC_MAX: 最亮环境的 ADC 值（直接对着灯/太阳时观测）
 */
#define AMBIENT_ADC_MIN     300U
#define AMBIENT_ADC_MAX     3500U

/* 自动亮度下背光的最小/最大百分比 */
#define BACK_LIGHT_MIN_PCT  10U   /* 避免完全黑屏：最低 10% */
#define BACK_LIGHT_MAX_PCT  100U  /* 最高亮度 100% */


/* ======= 3. 内部小工具函数 ======= */

/* 把 0..100% 亮度映射到 PWM CCR */
static void back_apply_percent(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_back_percent = percent;

    /* 计算 CCR：CCR = percent/100 * ARR */
    uint32_t ccr = (uint32_t)s_back_percent * s_pwm_max / 100U;

    __HAL_TIM_SET_COMPARE(BACK_TIM, BACK_TIM_CHANNEL, ccr);
}

/* 读取一次光敏 ADC 原始值（阻塞几个 us~ms，放在 GUI 任务里没问题） */
static uint16_t back_read_ambient_adc(void)
{
    HAL_ADC_Start(BACK_AMBIENT_ADC);
    if (HAL_ADC_PollForConversion(BACK_AMBIENT_ADC, 10) != HAL_OK) {
        HAL_ADC_Stop(BACK_AMBIENT_ADC);
        return s_ambient_raw;    /* 失败就返回上一次的值 */
    }

    uint32_t value = HAL_ADC_GetValue(BACK_AMBIENT_ADC);
    HAL_ADC_Stop(BACK_AMBIENT_ADC);

    if (value > 4095U) value = 4095U;
    return (uint16_t)value;
}

/* 把光敏 ADC 值映射为 10~100% 的亮度
 * 当前设定：环境越亮 -> 背光越亮（更容易看清）
 * 如果你想“越暗越亮”反向逻辑，可以把计算反过来。
 */
static uint8_t back_map_ambient_to_percent(uint16_t adc)
{
    /* 限幅到预设的 min/max 区间内 */
    if (adc <= AMBIENT_ADC_MIN) adc = AMBIENT_ADC_MIN;
    if (adc >= AMBIENT_ADC_MAX) adc = AMBIENT_ADC_MAX;

    uint32_t span_adc  = (uint32_t)(AMBIENT_ADC_MAX - AMBIENT_ADC_MIN);
    uint32_t span_back = (uint32_t)(BACK_LIGHT_MAX_PCT - BACK_LIGHT_MIN_PCT);

    uint32_t delta = (uint32_t)(adc - AMBIENT_ADC_MIN);
    uint32_t pct   = BACK_LIGHT_MIN_PCT + delta * span_back / span_adc;

    if (pct > 100U) pct = 100U;
    return (uint8_t)pct;
}


/* ======= 4. 对外 API 实现 ======= */

void Back_Init(void)
{
    /* 启动 PWM */
    HAL_TIM_PWM_Start(BACK_TIM, BACK_TIM_CHANNEL);

    /* 读取一次 ARR 作为 PWM 最大值 */
    s_pwm_max = __HAL_TIM_GET_AUTORELOAD(BACK_TIM);
    if (s_pwm_max == 0) s_pwm_max = 999;   /* 防止除零 */

    /* 默认亮度 50% */
    back_apply_percent(50);

    /* 初始化光敏滤波为当前环境 */
    s_ambient_raw = back_read_ambient_adc();
    s_ambient_lp  = (float)s_ambient_raw;
}

void Back_SetPercent(uint8_t percent)
{
    s_back_auto = 0;    /* 手动设置时自动关掉自动模式 */
    back_apply_percent(percent);
}

uint8_t Back_GetPercent(void)
{
    return s_back_percent;
}

void Back_SetAuto(uint8_t enable)
{
    s_back_auto = (enable ? 1 : 0);
}

uint8_t Back_GetAuto(void)
{
    return s_back_auto;
}

uint16_t Back_GetAmbientRaw(void)
{
    return s_ambient_raw;
}

/* 在 GUI 任务 / 其他周期任务里定期调用（例如每 500ms 一次） */
void Back_UpdateAuto(void)
{
    if (!s_back_auto) {
        return;     /* 手动模式就不动 */
    }

    /* 1) 读取一次光敏原始值 */
    uint16_t raw = back_read_ambient_adc();
    s_ambient_raw = raw;

    /* 2) 一阶 IIR 低通滤波，避免亮度抖动 */
    const float alpha = 0.1f;    /* 越大越敏感，越小越平滑 */
    if (s_ambient_lp == 0.0f) {
        s_ambient_lp = (float)raw;
    } else {
        s_ambient_lp += alpha * ((float)raw - s_ambient_lp);
    }

    /* 3) 映射为亮度百分比 */
    uint8_t target_pct = back_map_ambient_to_percent((uint16_t)s_ambient_lp);

    /* 4) 可以做一点“渐变”，避免一下子跳太大（可选） */
    uint8_t current = s_back_percent;
    if (target_pct > current) {
        if (target_pct - current > 5) {
            target_pct = current + 5;   /* 每次最多升 5% */
        }
    } else if (target_pct < current) {
        if (current - target_pct > 5) {
            target_pct = current - 5;   /* 每次最多降 5% */
        }
    }

    back_apply_percent(target_pct);
}
