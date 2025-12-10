#ifndef __BLOOD_H
#define __BLOOD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 对外给 GUI 用的数据结构 */
typedef struct {
    int32_t spo2;        // 血氧百分比
    int32_t heart_rate;  // 心率 bpm
    uint8_t spo2_valid;  // 1 = 有效
    uint8_t hr_valid;    // 1 = 有效
} SpO2_Data_t;

/* 全局变量：GUITask 直接读它 */
extern SpO2_Data_t g_spo2_data;

/* FreeRTOS 任务入口：在 freertos.c 里创建这个任务 */
void StartSpO2Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __BLOOD_H */




