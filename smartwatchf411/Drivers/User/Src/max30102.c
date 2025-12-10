#include "max30102.h"
#include "cmsis_os.h"

/* CubeMX 生成的 I2C1 句柄 */
extern I2C_HandleTypeDef hi2c1;
/* 你在 freertos.c 里创建的互斥锁 */
extern osMutexId_t I2C1_MutexHandle;

/* 内部工具函数：带互斥锁的 I2C 寄存器读写
 * 返回值：
 *   0 = 成功
 *   1 = 失败
 */
static uint8_t i2c_write_reg(uint8_t reg, uint8_t value)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK)
        return 1;

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(&hi2c1,
                                              MAX30102_I2C_ADDR,
                                              reg,
                                              I2C_MEMADD_SIZE_8BIT,
                                              &value,
                                              1,
                                              100);

    osMutexRelease(I2C1_MutexHandle);
    return (ret == HAL_OK) ? 0 : 1;
}

static uint8_t i2c_read_reg(uint8_t reg, uint8_t *value)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK)
        return 1;

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(&hi2c1,
                                             MAX30102_I2C_ADDR,
                                             reg,
                                             I2C_MEMADD_SIZE_8BIT,
                                             value,
                                             1,
                                             100);

    osMutexRelease(I2C1_MutexHandle);
    return (ret == HAL_OK) ? 0 : 1;
}

/* 对外封装 */
uint8_t max30102_write_reg(uint8_t reg, uint8_t value)
{
    return i2c_write_reg(reg, value);
}

uint8_t max30102_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_read_reg(reg, value);
}

uint8_t max30102_reset(void)
{
    /* MODE_CONFIG bit6 = 1 触发复位 */
    return max30102_write_reg(REG_MODE_CONFIG, 0x40);
}

/* 初始化配置：典型 SpO2 模式 + 100Hz 采样 */
uint8_t max30102_init(void)
{
    uint8_t status = 0;

    /* 1. 复位一下 */
    status |= max30102_reset();
    osDelay(10);

    /* 2. 中断配置：FIFO full / PPG ready 等 */
    status |= max30102_write_reg(REG_INTR_ENABLE_1, 0xC0);
    status |= max30102_write_reg(REG_INTR_ENABLE_2, 0x00);

    /* 3. FIFO 指针清零 */
    status |= max30102_write_reg(REG_FIFO_WR_PTR, 0x00);
    status |= max30102_write_reg(REG_OVF_COUNTER, 0x00);
    status |= max30102_write_reg(REG_FIFO_RD_PTR, 0x00);

    /* 4. FIFO 配置：sample avg=1, rollover=0, almost full=15 */
    status |= max30102_write_reg(REG_FIFO_CONFIG, 0x0F);

    /* 5. 模式：SpO2 模式（红光 + IR） */
    status |= max30102_write_reg(REG_MODE_CONFIG, 0x03);

    /* 6. SPO2 配置：4096nA, 100Hz, 400us */
    status |= max30102_write_reg(REG_SPO2_CONFIG, 0x27);

    /* 7. LED 电流 */
    status |= max30102_write_reg(REG_LED1_PA, 0x24);  // RED
    status |= max30102_write_reg(REG_LED2_PA, 0x24);  // IR
    status |= max30102_write_reg(REG_PILOT_PA, 0x7F); // Pilot

    /* 只要有一个步骤失败，status 就会是非 0，统一当作失败返回 1 */
    return (status == 0) ? 0 : 1;
}

/* 一次读 RED/IR 两个样本，24bit -> 18bit 有效 */
uint8_t max30102_read_fifo(uint32_t *red, uint32_t *ir)
{
    if (!red || !ir) return 1;

    uint8_t buf[6];
    uint8_t dummy;

    /* 读一下中断状态寄存器，把中断标志清一下（可选） */
    max30102_read_reg(REG_INTR_STATUS_1, &dummy);
    max30102_read_reg(REG_INTR_STATUS_2, &dummy);

    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK)
        return 1;

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(&hi2c1,
                                             MAX30102_I2C_ADDR,
                                             REG_FIFO_DATA,
                                             I2C_MEMADD_SIZE_8BIT,
                                             buf,
                                             6,
                                             100);

    osMutexRelease(I2C1_MutexHandle);

    if (ret != HAL_OK) return 1;

    uint32_t raw_red = ((uint32_t)buf[0] << 16) |
                       ((uint32_t)buf[1] << 8)  |
                        (uint32_t)buf[2];

    uint32_t raw_ir  = ((uint32_t)buf[3] << 16) |
                       ((uint32_t)buf[4] << 8)  |
                        (uint32_t)buf[5];

    *red = raw_red & 0x03FFFFU;   // 18bit 有效
    *ir  = raw_ir  & 0x03FFFFU;

    return 0;
}




