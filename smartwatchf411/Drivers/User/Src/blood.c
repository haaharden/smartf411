#include "blood.h"
#include "max30102.h"
#include "algorithm.h"
#include "cmsis_os.h"

SpO2_Data_t g_spo2_data = {0};

/**
 * @brief  MAX30102 + 算法任务
 * @note   思路：
 *         1. 周期性检查中断标志，有数据就读 FIFO。
 *         2. 把 RED/IR 推进缓冲数组。
 *         3. 满 500 点（5 秒）就调用一次算法，更新 g_spo2_data。
 *         4. 每次循环都 osDelay(5)，不阻塞 FreeRTOS。
 */
void StartSpO2Task(void *argument)
{
    static uint32_t ir_buf[BUFFER_SIZE];
    static uint32_t red_buf[BUFFER_SIZE];
    int buf_idx = 0;

    int32_t spo2;
    int8_t  spo2_valid;
    int32_t heart_rate;
    int8_t  hr_valid;

    /* 初始化 MAX30102 */
    if (max30102_init() != 0) {
        // 这里可以打印一下错误，但不要死循环
        // printf("MAX30102 init failed\r\n");
    }

    for (;;)
    {
        int samples_this_loop = 0;

        /* 一次循环最多处理几个 FIFO 样本，避免长时间占住 I2C */
        while (samples_this_loop < 4)
        {
            uint8_t intr1;
            if (max30102_read_reg(REG_INTR_STATUS_1, &intr1) != 0) {
                break;  // I2C 错了，下次再试
            }

            if (intr1 & 0x40) {   // A_FULL or PPG_RDY 之类的新数据标志
                uint32_t red, ir;
                if (max30102_read_fifo(&red, &ir) == 0)
                {
                    ir_buf[buf_idx]  = ir;
                    red_buf[buf_idx] = red;
                    buf_idx++;
                    samples_this_loop++;

                    if (buf_idx >= BUFFER_SIZE)
                    {
                        /* 调算法，算一次 HR + SpO2 */
                        maxim_heart_rate_and_oxygen_saturation(
                            ir_buf,
                            red_buf,
                            BUFFER_SIZE,
                            &spo2,
                            &spo2_valid,
                            &heart_rate,
                            &hr_valid);

                        if (spo2_valid) {
                            g_spo2_data.spo2       = spo2;
                            g_spo2_data.spo2_valid = 1;
                        } else {
                            g_spo2_data.spo2_valid = 0;
                        }

                        if (hr_valid) {
                            g_spo2_data.heart_rate = heart_rate;
                            g_spo2_data.hr_valid   = 1;
                        } else {
                            g_spo2_data.hr_valid   = 0;
                        }

                        /* 做个滑动窗口：保留一半旧数据，响应更快 */
                        int keep = BUFFER_SIZE / 2;   // 保留 2.5 秒
                        for (int i = 0; i < keep; i++) {
                            ir_buf[i]  = ir_buf[BUFFER_SIZE - keep + i];
                            red_buf[i] = red_buf[BUFFER_SIZE - keep + i];
                        }
                        buf_idx = keep;
                    }
                }
                else {
                    break;  // 读 FIFO 失败
                }
            }
            else {
                /* 没有新数据，结束本轮 while */
                break;
            }
        }

        /* 一定要让出 CPU，避免把 I2C/CPU 占死 */
        osDelay(5);   // 5 ms，理论上 10ms 一次采就能跟上 100Hz
    }
}

