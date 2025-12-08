#include "mpu6500.h"
#include "cmsis_os.h"

extern osMutexId_t I2C1_MutexHandle;

ImuData_t g_imu_data = {0};

// 7bit 地址 0x68，HAL 里要左移一位（这个宏在 .h 里已经定义了）
// #define MPU6500_I2C_ADDR     (0x68 << 1)

static bool mpu6500_write_reg(uint8_t reg, uint8_t data)
{
	  if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return false;   // 或者返回一个特殊值
    }

    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(&hi2c1,MPU6500_I2C_ADDR,reg,I2C_MEMADD_SIZE_8BIT,&data,
                               1,
                               100);
		osMutexRelease(I2C1_MutexHandle);
    return (status == HAL_OK);
}

static bool mpu6500_read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
		if (osMutexAcquire(I2C1_MutexHandle, osWaitForever) != osOK) {
        return false;   // 或者返回一个特殊值
    }
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read(&hi2c1,MPU6500_I2C_ADDR,reg,I2C_MEMADD_SIZE_8BIT,buf,len,100);
		osMutexRelease(I2C1_MutexHandle);
    return (status == HAL_OK);
}

bool MPU6500_Init(void)
{
    uint8_t whoami = 0;

    // 读 WHO_AM_I 检查芯片 ID
    if (!mpu6500_read_regs(MPU6500_REG_WHO_AM_I, &whoami, 1)) {
        return false;
    }

    // 简单检查：不为 0x00/0xFF 就先认为 I2C 正常
    if (whoami == 0x00 || whoami == 0xFF) {
        return false;
    }

    // 退出睡眠，选择时钟（使用 X 轴陀螺作为时钟源）
    if (!mpu6500_write_reg(MPU6500_REG_PWR_MGMT_1, 0x01))  // CLKSEL=1, SLEEP=0
        return false;

    // 采样分频器，陀螺基准 1kHz：采样率 = 1kHz / (1 + SMPLRT_DIV)
    if (!mpu6500_write_reg(MPU6500_REG_SMPLRT_DIV, 9))     // => 100 Hz
        return false;

    // DLPF 配置（CONFIG），0x03 大约 44Hz 带宽
    if (!mpu6500_write_reg(MPU6500_REG_CONFIG, 0x03))
        return false;

    // 陀螺量程 ±500°/s (FS_SEL=1)
    if (!mpu6500_write_reg(MPU6500_REG_GYRO_CONFIG, 0x08))
        return false;

    // 加速度量程 ±4g (AFS_SEL=1)
    if (!mpu6500_write_reg(MPU6500_REG_ACCEL_CONFIG, 0x08))
        return false;

    // 加速度 DLPF 设置
    if (!mpu6500_write_reg(MPU6500_REG_ACCEL_CONFIG2, 0x03))
        return false;

    // 中断脚配置：这里先用默认，不开中断
    if (!mpu6500_write_reg(MPU6500_REG_INT_PIN_CFG, 0x00))
        return false;

    if (!mpu6500_write_reg(MPU6500_REG_INT_ENABLE, 0x00))
        return false;

    return true;
}

bool MPU6500_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[14];

    if (!mpu6500_read_regs(MPU6500_REG_ACCEL_XOUT_H, buf, sizeof(buf)))
        return false;

    int16_t ax_raw = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay_raw = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az_raw = (int16_t)((buf[4] << 8) | buf[5]);
    // buf[6],[7] 是温度
    int16_t gx_raw = (int16_t)((buf[8] << 8)  | buf[9]);
    int16_t gy_raw = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz_raw = (int16_t)((buf[12] << 8) | buf[13]);

    if (ax) *ax = ax_raw;
    if (ay) *ay = ay_raw;
    if (az) *az = az_raw;
    if (gx) *gx = gx_raw;
    if (gy) *gy = gy_raw;
    if (gz) *gz = gz_raw;

    return true;
}

bool MPU6500_ReadAndConvert(ImuData_t *out)
{
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx_raw, gy_raw, gz_raw;

    if (!MPU6500_ReadRaw(&ax_raw, &ay_raw, &az_raw,
                         &gx_raw, &gy_raw, &gz_raw))
        return false;

    // 量程对应的 LSB：±4g => 8192 LSB/g，±500°/s => 65.5 LSB/(°/s)
    const float ACCEL_LSB_4G = 8192.0f;
    const float GYRO_LSB_500 = 65.5f;

    out->ax = ax_raw / ACCEL_LSB_4G;
    out->ay = ay_raw / ACCEL_LSB_4G;
    out->az = az_raw / ACCEL_LSB_4G;

    out->gx = gx_raw / GYRO_LSB_500;
    out->gy = gy_raw / GYRO_LSB_500;
    out->gz = gz_raw / GYRO_LSB_500;

    // 顺带更新全局
    g_imu_data = *out;

    return true;
}
