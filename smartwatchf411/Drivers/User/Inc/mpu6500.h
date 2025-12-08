#ifndef __MPU6500_H
#define __MPU6500_H

#include "main.h"
#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 7bit 地址 0x68 / 0x69，HAL 里要左移一位
#define MPU6500_I2C_ADDR     (0x68 << 1)

// 寄存器地址
#define MPU6500_REG_PWR_MGMT_1   0x6B
#define MPU6500_REG_SMPLRT_DIV   0x19
#define MPU6500_REG_CONFIG       0x1A
#define MPU6500_REG_GYRO_CONFIG  0x1B
#define MPU6500_REG_ACCEL_CONFIG 0x1C
#define MPU6500_REG_ACCEL_CONFIG2 0x1D
#define MPU6500_REG_INT_PIN_CFG  0x37
#define MPU6500_REG_INT_ENABLE   0x38
#define MPU6500_REG_WHO_AM_I     0x75

#define MPU6500_REG_ACCEL_XOUT_H 0x3B  // 连续 14 字节：Ax,Ay,Az,Temp,Gx,Gy,Gz

typedef struct {
    float ax, ay, az;   // 单位 g
    float gx, gy, gz;   // 单位 °/s
} ImuData_t;

// 全局最新 IMU 数据（方便 GUI 和其他任务使用）
extern ImuData_t g_imu_data;

// 初始化 & 读数据
bool MPU6500_Init(void);
bool MPU6500_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz);
bool MPU6500_ReadAndConvert(ImuData_t *out);

#ifdef __cplusplus
}
#endif

#endif
