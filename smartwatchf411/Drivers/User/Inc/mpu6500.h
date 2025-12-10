#ifndef __MPU6500_H
#define __MPU6500_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"
#include <stdint.h>

/* I2C 地址：MPU6500/6050 7bit 地址是 0x68，在 HAL 里要左移一位 */
#define MPU6500_I2C_ADDR          (0x68U << 1)

/* 寄存器地址 */
#define MPU6500_REG_SMPLRT_DIV    0x19
#define MPU6500_REG_CONFIG        0x1A
#define MPU6500_REG_GYRO_CONFIG   0x1B
#define MPU6500_REG_ACCEL_CONFIG  0x1C
#define MPU6500_REG_ACCEL_CONFIG2 0x1D

#define MPU6500_REG_INT_PIN_CFG   0x37
#define MPU6500_REG_INT_ENABLE    0x38

#define MPU6500_REG_ACCEL_XOUT_H  0x3B
#define MPU6500_REG_TEMP_OUT_H    0x41
#define MPU6500_REG_GYRO_XOUT_H   0x43

#define MPU6500_REG_PWR_MGMT_1    0x6B
#define MPU6500_REG_WHO_AM_I      0x75

/* 量程转换系数：根据配置选择的量程而定
 * 这里配置为：加速度 ±2g，陀螺 ±2000 dps
 */
#define MPU6500_ACC_LSB_2G        16384.0f     /* 1g = 16384 LSB */
#define MPU6500_GYRO_LSB_2000DPS  16.4f        /* 1 dps = 16.4 LSB */

/* IMU 数据结构，给外部任务/界面使用 */
typedef struct {
    float ax;      /* 单位：g */
    float ay;
    float az;

    float gx;      /* 单位：deg/s */
    float gy;
    float gz;

    float temperature;   /* 单位：摄氏度 */
} ImuData_t;

/* 全局 IMU 数据 */
extern ImuData_t g_imu_data;

/* 用户活动状态枚举 */
typedef enum {
    IMU_ACTIVITY_UNKNOWN = 0,   /* 未知 / 初始化状态 */

    IMU_ACTIVITY_REST,         /* 静止 / 躺着 / 睡觉候选 */
    IMU_ACTIVITY_LIGHT,        /* 轻微活动（站立、轻微晃动） */
    IMU_ACTIVITY_WALK,         /* 走路 */
    IMU_ACTIVITY_RUN           /* 跑步 / 剧烈活动 */
} ImuActivity_t;

/* 当前识别出的用户状态 */
extern ImuActivity_t g_imu_activity;

/* 接口函数 */
uint8_t MPU6500_Init(void);
uint8_t MPU6500_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                        int16_t *gx, int16_t *gy, int16_t *gz,
                        int16_t *temp);
void MPU6500_Update(void);

/* 活动识别更新函数：在 IMU 任务中周期性调用 */
void IMU_Activity_Update(void);

/* 获取当前活动状态（读取 g_imu_activity） */
ImuActivity_t IMU_GetActivity(void);

/* FreeRTOS 任务入口函数（在 freertos.c 里用 osThreadNew 调用） */
void StartIMUTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6500_H */

