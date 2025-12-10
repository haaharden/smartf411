#ifndef __ALGORITHM_H
#define __ALGORITHM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 采样频率（和 MAX30102 配置保持一致） */
#define FS                100            // 100 Hz
/* 每次算法使用多少点：这里用 5 秒数据 */
#define BUFFER_SIZE       (FS * 5)       // 500 点

/**
 * @brief   从一段红光/红外波形中估算心率和血氧
 * @param   ir_buffer       红外通道原始数据数组（长度 = buffer_length）
 * @param   red_buffer      红光通道原始数据数组
 * @param   buffer_length   数组长度（建议 >= BUFFER_SIZE）
 * @param   spo2            输出：血氧（百分比）
 * @param   spo2_valid      输出：1=有效，0=无效
 * @param   heart_rate      输出：心率（bpm）
 * @param   hr_valid        输出：1=有效，0=无效
 */
void maxim_heart_rate_and_oxygen_saturation(
    const uint32_t *ir_buffer,
    const uint32_t *red_buffer,
    int32_t buffer_length,
    int32_t *spo2,
    int8_t  *spo2_valid,
    int32_t *heart_rate,
    int8_t  *hr_valid);

#ifdef __cplusplus
}
#endif

#endif /* __ALGORITHM_H */
