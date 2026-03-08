#include "blood.h"
#include "max30102.h"
#include "algorithm.h"
#include "cmsis_os.h"
#include "stdbool.h"

#define LPF_ALPHA  0.2f

SpO2_Data_t g_spo2_data = {0};

/*
				MAX30102 + 算法任务
        1. 周期性检查中断标志，有数据就读 FIFO。
        2. 把 RED/IR 推进缓冲数组。
        3. 满 500 点（5 秒）就调用一次算法，更新 g_spo2_data。
        4. 每次循环都 osDelay(5)，不阻塞 FreeRTOS。
 */
void StartSpO2Task(void *argument)
{
    static uint32_t ir_buf[BUFFER_SIZE];
    static uint32_t red_buf[BUFFER_SIZE];
    static float f_red = 0.0f;
    static float f_ir  = 0.0f;
    static bool is_first_sample = true;

    int buf_idx = 0;
    int32_t spo2, heart_rate;
    int8_t  spo2_valid, hr_valid;

    /* 初始化 MAX30102 */
    if (max30102_init() != 0) {
        // 初始化失败逻辑
    }

    for (;;)
    {
        int samples_this_loop = 0;

        /* 一次循环最多处理几个 FIFO 样本，避免长时间占住 I2C */
        while (samples_this_loop < 4)
        {
            uint8_t intr1;
            if (max30102_read_reg(REG_INTR_STATUS_1, &intr1) != 0) break;

            if (intr1 & 0x40) {   // 有新数据
                uint32_t raw_red, raw_ir;
                if (max30102_read_fifo(&raw_red, &raw_ir) == 0)
                {
                    /* --- 一阶低通滤波处理 --- */
                    if (is_first_sample) {
                        f_red = (float)raw_red;
                        f_ir  = (float)raw_ir;
                        is_first_sample = false;
                    } else {
                        f_red = LPF_ALPHA * (float)raw_red + (1.0f - LPF_ALPHA) * f_red;
                        f_ir  = LPF_ALPHA * (float)raw_ir  + (1.0f - LPF_ALPHA) * f_ir;
                    }

                    // 将滤波后的值存入数组供算法使用
                    ir_buf[buf_idx]  = (uint32_t)f_ir;
                    red_buf[buf_idx] = (uint32_t)f_red;
                    
                    buf_idx++;
                    samples_this_loop++;

                    /* --- 缓冲区满，执行算法 --- */
                    if (buf_idx >= BUFFER_SIZE)
                    {
                        maxim_heart_rate_and_oxygen_saturation(
                            ir_buf, red_buf, BUFFER_SIZE,
                            &spo2, &spo2_valid, &heart_rate, &hr_valid);

                        // 更新全局数据
                        g_spo2_data.spo2       = spo2_valid ? spo2 : 0;
                        g_spo2_data.spo2_valid = spo2_valid;
                        g_spo2_data.heart_rate = hr_valid   ? heart_rate : 0;
                        g_spo2_data.hr_valid   = hr_valid;

                        /* 移动窗口：保留一半旧数据 */
                        int keep = BUFFER_SIZE / 2;
                        for (int i = 0; i < keep; i++) {
                            ir_buf[i]  = ir_buf[BUFFER_SIZE - keep + i];
                            red_buf[i] = red_buf[BUFFER_SIZE - keep + i];
                        }
                        buf_idx = keep;
                    }
                } else {
                    break;
                }
            } else {
                break; // 无新数据
            }
        }
        osDelay(5); // 释放 CPU，给 LVGL 或其他任务运行时间
    }
}

