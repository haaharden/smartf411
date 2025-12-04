#include "i2c.h"
#include "gpio.h"
#include "touch.h"
#include "stm32f4xx_it.h"

/*	TP_SCL<->PB6
		TP_SDA<->PB7
		TP_INT<->PB9
		TP_RST<->PB8	*/

volatile uint8_t touch_int_flag = 0;//中断标志

extern I2C_HandleTypeDef hi2c1;

/*------------------ 内部小工具函数：I2C 读写 ------------------*/

// 写单字节寄存器
static void CST816_WriteReg(uint8_t reg, uint8_t value)
{
    HAL_I2C_Mem_Write(&hi2c1,
                      CST816_ADDRESS,           // 0x2A：已经是 8bit 地址，不要再左移
                      reg,
                      I2C_MEMADD_SIZE_8BIT,
                      &value,
                      1,
                      100);
}

// 连续读多个寄存器
static uint8_t CST816_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (HAL_I2C_Mem_Read(&hi2c1,
                         CST816_ADDRESS,
                         reg,
                         I2C_MEMADD_SIZE_8BIT,
                         buf,
                         len,
                         100) == HAL_OK)
    {
        return 1;
    }
    return 0;
}

/*------------------ 1. 中断回调：把“有触摸了”记下来 ------------------*/

// EXTI 中断回调，在 stm32f4xx_it.c 里调用 HAL_GPIO_EXTI_IRQHandler 后自动进这里
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TP_INT_Pin)   // TP_INT_Pin 在 main.h 里，一般就是 GPIO_PIN_x
    {
        // 只做简单的标志位，不在中断里读 I2C
        touch_int_flag = 1;
				HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    }
}

/*------------------ 2. 初始化触摸芯片 ------------------*/

void CST816T_Init(void)
{
    // ① 硬件复位：拉低 RST 再拉高
    //    这里假设你在 CubeMX 里把 TP_RST 配成了普通推挽输出
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);

    // ②（可选）关自动休眠、配置中断行为
    // 根据 CST816S/T 寄存器手册：
    // DisAutoSleep(0xFE) 非 0 => 禁止自动进低功耗
    CST816_WriteReg(DisAutoSleep, 0x01);

    // AutoSleepTime(0xF9) 默认 2s，这里随便设一个稍大的，单位 1s
    CST816_WriteReg(AutoSleepTime, 5);

    // IrqCrl(0xFA)：使能 Touch/Change/Motion 中断
    // Bit6 EnTouch, Bit5 EnChange, Bit4 EnMotion
    // 0x70 = 0b0111_0000，对大部分场景够用
    CST816_WriteReg(IrqCrl, 0x70);

    // ③（可选）读一下芯片 ID，验证 I2C 是否正常
    uint8_t id = 0;
    if (CST816_ReadRegs(ChipID, &id, 1))
    {
        // 你可以在这里 printf 看一下 ID 值，用于调试
        // printf("CST816 ChipID: 0x%02X\r\n", id);
    }

    // ④ 读一次 GestureID，把上电残留的中断状态清掉
    uint8_t dummy;
    CST816_ReadRegs(GestureID, &dummy, 1);
}

/*------------------ 3. 读取一次坐标和手势 ------------------*/
/*
 * 功能：
 *   在 touch_int_flag 置位之后调用，从 CST816T 中读出：
 *   - 手势 GestureID（0x01）
 *   - 手指数量 FingerNum（0x02）
 *   - X/Y 坐标（0x03~0x06）
 *
 * 参数：
 *   X, Y      -> 返回坐标
 *   Gesture   -> 返回手势代码（0x05 单击，0x0B 双击，0x01~0x04 上下左右滑 等）
 *
 * 返回值：
 *   1  -> 有有效触摸（至少 1 个手指）
 *   0  -> 没有有效触摸（FingerNum=0 或 I2C 读失败）
 */
uint8_t CST816_GetAction(uint16_t *X, uint16_t *Y, uint8_t *Gesture)
{
    uint8_t buf[6];

    // 从 GestureID(0x01) 连续读到 YposL(0x06)
    if (!CST816_ReadRegs(GestureID, buf, sizeof(buf)))
    {
        return 0;   // I2C 读失败
    }

    uint8_t gesture = buf[0];      // GestureID
    uint8_t fingers = buf[1] & 0x0F; // FingerNum（低 4 位）

    if (fingers == 0)
    {
        // 手指数量为 0，说明当前没有触摸
        return 0;
    }

    // 解析坐标：高 4 位在 XposH/YposH 低 4bit，低 8 位在 XposL/YposL
    uint16_t x = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((uint16_t)(buf[4] & 0x0F) << 8) | buf[5];

    if (X) *X = x;
    if (Y) *Y = y;
    if (Gesture) *Gesture = gesture;

    return 1;
}

/*------------------ 4. 把原始手势映射成你的枚举事件 ------------------*/
/*
 * 功能：
 *   根据触摸芯片返回的 gesture & status（比如手指数量），
 *   转成你自己定义的 TouchEvent（无事件 / 单击 / 双击）。
 *
 * 参数：
 *   x, y     -> 坐标（这里先不用，将来可以做“点击某个区域触发不同功能”）
 *   gesture -> GestureID 寄存器的值（0x01、0x02、0x05、0x0B 等）
 *   status  -> 建议传 FingerNum（0: 无手指，1: 有手指）
 *
 * 返回值：
 *   EVENT_NONE         -> 没有有意义的点击
 *   EVENT_SINGLE_CLICK -> 单击
 *   EVENT_DOUBLE_CLICK -> 双击
 *
 * 说明：
 *   根据官方寄存器说明：
 *     GestureID = 0x05 -> Single click
 *     GestureID = 0x0B -> Double click
 *     GestureID = 0x0C -> Long press
 */
TouchEvent GetTouchEvent(uint16_t x, uint16_t y, uint8_t gesture, uint8_t status)
{
    (void)x; // 先不用坐标，避免未使用警告
    (void)y;

    if (status == 0)
    {
        return EVENT_NONE;  // 没有手指触摸
    }

    switch (gesture)
    {
        case 0x05:  // Single click
            return EVENT_SINGLE_CLICK;

        case 0x0B:  // Double click
            return EVENT_DOUBLE_CLICK;

        default:
            return EVENT_NONE;
    }
}

