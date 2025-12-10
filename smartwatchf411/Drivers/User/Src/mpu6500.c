#include "mpu6500.h"
#include "cmsis_os.h"
#include <math.h>

extern I2C_HandleTypeDef hi2c1;
extern osMutexId_t I2C1_MutexHandle;

/* 对外的全局 IMU 数据 */
ImuData_t g_imu_data = {0};

/* 内部 I2C 读写封装，带互斥锁 */
static uint8_t mpu6500_write_reg(uint8_t reg, uint8_t data)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1,
                                                 MPU6500_I2C_ADDR,
                                                 reg,
                                                 I2C_MEMADD_SIZE_8BIT,
                                                 &data,
                                                 1,
                                                 100);
    osMutexRelease(I2C1_MutexHandle);

    return (status == HAL_OK) ? 1 : 0;
}

static uint8_t mpu6500_read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1,
                                                MPU6500_I2C_ADDR,
                                                reg,
                                                I2C_MEMADD_SIZE_8BIT,
                                                buf,
                                                len,
                                                100);
    osMutexRelease(I2C1_MutexHandle);

    return (status == HAL_OK) ? 1 : 0;
}

/* MPU6500 基本初始化：
 * - 唤醒，使用内部时钟
 * - 设置采样率、低通滤波
 * - 配置加速度和陀螺量程
 */
uint8_t MPU6500_Init(void)
{
    uint8_t whoami = 0;

    /* 读 WHO_AM_I 检查芯片 ID（MPU6500 典型为 0x70，MPU6050 为 0x68） */
    if (!mpu6500_read_regs(MPU6500_REG_WHO_AM_I, &whoami, 1)) {
        return 0;
    }

    if (whoami != 0x70 && whoami != 0x68) {
        /* 如果你确定就是 6500，可以在这里打印错误或直接返回失败 */
        // printf("MPU6500 WHO_AM_I = 0x%02X\r\n", whoami);
        // return 0;
    }

    /* 软复位 */
    mpu6500_write_reg(MPU6500_REG_PWR_MGMT_1, 0x80);
    HAL_Delay(100);

    /* 选择内部时钟，退出休眠 */
    mpu6500_write_reg(MPU6500_REG_PWR_MGMT_1, 0x01);  // 时钟源：PLL X 轴

    /* 采样率：SampleRate = GyroOutputRate / (1 + SMPLRT_DIV)
     * Gyro 典型输出 1kHz，这里设置为 100Hz：1k / (1+9) = 100
     */
    mpu6500_write_reg(MPU6500_REG_SMPLRT_DIV, 9);

    /* 配置陀螺低通滤波，DLPF_CFG=3（约 44Hz） */
    mpu6500_write_reg(MPU6500_REG_CONFIG, 0x03);

    /* 陀螺量程：GYRO_CONFIG[4:3] = 3 => ±2000 dps */
    mpu6500_write_reg(MPU6500_REG_GYRO_CONFIG, 0x18);

    /* 加速度量程：ACCEL_CONFIG[4:3] = 0 => ±2g */
    mpu6500_write_reg(MPU6500_REG_ACCEL_CONFIG, 0x00);

    /* 加速度低通滤波 */
    mpu6500_write_reg(MPU6500_REG_ACCEL_CONFIG2, 0x03);

    /* 中断配置（可选）：INT 引脚推挽 / 低电平有效等，这里简单开 FIFO 数据就绪中断 */
    mpu6500_write_reg(MPU6500_REG_INT_PIN_CFG, 0x00);
    mpu6500_write_reg(MPU6500_REG_INT_ENABLE, 0x01);   // Data Ready interrupt

    return 1;
}

/* 读取 14 字节原始数据：Accel(6) + Temp(2) + Gyro(6) */
uint8_t MPU6500_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                        int16_t *gx, int16_t *gy, int16_t *gz,
                        int16_t *temp)
{
    uint8_t buf[14];

    if (!mpu6500_read_regs(MPU6500_REG_ACCEL_XOUT_H, buf, sizeof(buf))) {
        return 0;
    }

    int16_t _ax   = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t _ay   = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t _az   = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t _temp = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t _gx   = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t _gy   = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t _gz   = (int16_t)((buf[12] << 8) | buf[13]);

    if (ax)   *ax   = _ax;
    if (ay)   *ay   = _ay;
    if (az)   *az   = _az;
    if (gx)   *gx   = _gx;
    if (gy)   *gy   = _gy;
    if (gz)   *gz   = _gz;
    if (temp) *temp = _temp;

    return 1;
}

/* 将原始数据转换为物理量并写入全局 g_imu_data
 * 供其他任务（比如姿态解算、UI 显示）使用
 */
void MPU6500_Update(void)
{
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx_raw, gy_raw, gz_raw;
    int16_t temp_raw;

    if (!MPU6500_ReadRaw(&ax_raw, &ay_raw, &az_raw,
                         &gx_raw, &gy_raw, &gz_raw,
                         &temp_raw)) {
        return;
    }

    /* 按照配置的量程转换成物理单位 */
    g_imu_data.ax = (float)ax_raw / MPU6500_ACC_LSB_2G;
    g_imu_data.ay = (float)ay_raw / MPU6500_ACC_LSB_2G;
    g_imu_data.az = (float)az_raw / MPU6500_ACC_LSB_2G;

    g_imu_data.gx = (float)gx_raw / MPU6500_GYRO_LSB_2000DPS;
    g_imu_data.gy = (float)gy_raw / MPU6500_GYRO_LSB_2000DPS;
    g_imu_data.gz = (float)gz_raw / MPU6500_GYRO_LSB_2000DPS;

    /* 温度换算：MPU6500/6050 典型公式：Temp(C) = (temp_raw / 333.87) + 21 */
    g_imu_data.temperature = ((float)temp_raw) / 333.87f + 21.0f;
}

