#include "i2c.h"
#include "gpio.h"
#include "touch.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include <stdio.h>
#include "cmsis_os.h"

extern osMutexId_t I2C1_MutexHandle;

/*	TP_SCL<->PB6
		TP_SDA<->PB7
		TP_INT<->PB9
		TP_RST<->PB8	*/

volatile uint8_t touch_int_flag = 0;//中断标志

extern I2C_HandleTypeDef hi2c1;

/*------------------ 内部小工具函数：I2C 读写 ------------------*/

// 写单字节寄存器
uint8_t CST816_WriteReg(uint8_t reg, uint8_t data)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1,
                                                CST816_ADDRESS,
                                                reg,
                                                I2C_MEMADD_SIZE_8BIT,
                                                &data,
                                                1,
                                                100);

    osMutexRelease(I2C1_MutexHandle);

    if (status != HAL_OK)
    {
        I2C_BusRecover(&hi2c1); // 写入失败也复位
        return 0;
    }
    return 1;
}


// 连续读多个寄存器
uint8_t CST816_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1,
                                               CST816_ADDRESS,
                                               reg,
                                               I2C_MEMADD_SIZE_8BIT,
                                               buf,
                                               len,
                                               100);

    osMutexRelease(I2C1_MutexHandle);

    if (status != HAL_OK)
    {
        printf("Touch I2C Fail (Error=%d)! Recovering...\r\n", status);
        
        // 调用你刚才写的通用复位函数，救活 I2C1
        I2C_BusRecover(&hi2c1); 
        
        return 0; // 返回错误，让上层下次再试
    }
    return 1;
}



// EXTI 中断回调，在 stm32f4xx_it.c 里调用 HAL_GPIO_EXTI_IRQHandler 后自动进这里
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TP_INT_Pin)   // TP_INT_Pin 在 main.h
    {
        // 只做简单的标志位，不在中断里读 I2C
				//注意，flag只能判断是不是抬起和松手的状态变了，不能判断手是否在屏幕上
        touch_int_flag = 1;
				//HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    }
}

/*------------------ 2. 初始化触摸芯片 ------------------*/

void CST816T_Init(void)
{
    // 硬件复位：拉低 RST 再拉高
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);

    // 关自动休眠、配置中断行为
    // 根据 CST816T 寄存器手册：
    // DisAutoSleep(0xFE) 非 0 => 禁止自动进低功耗
    CST816_WriteReg(DisAutoSleep, 0x01);

    // AutoSleepTime(0xF9) 默认 2s，这里随便设一个稍大的，单位 1s
    CST816_WriteReg(AutoSleepTime, 5);

    // IrqCrl(0xFA)：使能 Touch/Change/Motion 中断
    // Bit6 EnTouch, Bit5 EnChange, Bit4 EnMotion
    // 0x70 = 0b0111_0000，对大部分场景够用
    CST816_WriteReg(IrqCrl, 0x70);

    // 读一下芯片 ID，验证 I2C 是否正常
    uint8_t id = 0;
    if (CST816_ReadRegs(ChipID, &id, 1))
    {
        // 可以在这里 printf 看一下 ID 值，用于调试
        // printf("CST816 ChipID: 0x%02X\r\n", id);
    }

    // 读一次 GestureID，把上电残留的中断状态清掉
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
uint8_t CST816_GetAction(uint16_t *X, uint16_t *Y,uint8_t *Gesture, uint8_t *pFingerNum)
{
    uint8_t buf[6];

    if (!CST816_ReadRegs(GestureID, buf, sizeof(buf))) {
        return 0;
    }

    uint8_t gesture = buf[0];          // GestureID
    uint8_t fingers = buf[1] & 0x0F;   // FingerNum

    if (fingers == 0) {
        return 0;
    }

    uint16_t x = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((uint16_t)(buf[4] & 0x0F) << 8) | buf[5];

    if (X) *X = x;
    if (Y) *Y = y;
    if (Gesture)   *Gesture   = gesture;
    if (pFingerNum) *pFingerNum = fingers;

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
TouchEvent GetTouchEvent(uint16_t x, uint16_t y, uint8_t gesture, uint8_t fingers)
{
    (void)x;
    (void)y;

    if (fingers == 0) {
        return EVENT_NONE;    // 没触摸
    }

    switch (gesture)
    {
        case 0x05:  // Single click
            return EVENT_SINGLE_CLICK;

        case 0x0B:  // Double click
            return EVENT_DOUBLE_CLICK;

        case 0x0C:  // Long press
            return EVENT_LONG_PRESS;

        case 0x01:  // Up
            return EVENT_SLIDE_UP;

        case 0x02:  // Down
            return EVENT_SLIDE_DOWN;

        case 0x03:  // Left
            return EVENT_SLIDE_LEFT;

        case 0x04:  // Right
            return EVENT_SLIDE_RIGHT;

        default:
            return EVENT_NONE;
    }
}

