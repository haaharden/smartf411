#include "algorithm.h"
#include <math.h>

/* 简单排序，用于求 R 的中值（冒泡就够了，数据量很小） */
static void sort_float(float *arr, int n)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (arr[j] > arr[j + 1]) {
                float t = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = t;
            }
}

void maxim_heart_rate_and_oxygen_saturation(
    const uint32_t *ir_buffer,
    const uint32_t *red_buffer,
    int32_t buffer_length,
    int32_t *spo2,
    int8_t  *spo2_valid,
    int32_t *heart_rate,
    int8_t  *hr_valid)
{
    if (!ir_buffer || !red_buffer ||
        !spo2 || !spo2_valid || !heart_rate || !hr_valid ||
        buffer_length <= 0) {
        if (spo2_valid) *spo2_valid = 0;
        if (hr_valid)   *hr_valid   = 0;
        return;
    }

    if (buffer_length > BUFFER_SIZE) {
        buffer_length = BUFFER_SIZE;
    }

    static float ir_work[BUFFER_SIZE];
    static float red_work[BUFFER_SIZE];
    static float ir_filt[BUFFER_SIZE];

    /* 1. 复制数据，并计算 IR 的 DC + 幅度 */
    float ir_mean = 0.0f;
    float ir_min  = (float)ir_buffer[0];
    float ir_max  = (float)ir_buffer[0];

    for (int i = 0; i < buffer_length; i++) {
        float irv  = (float)ir_buffer[i];
        float redv = (float)red_buffer[i];

        ir_work[i]  = irv;
        red_work[i] = redv;

        ir_mean += irv;
        if (irv < ir_min) ir_min = irv;
        if (irv > ir_max) ir_max = irv;
    }
    ir_mean /= (float)buffer_length;

    /* 如果 IR 波形几乎没有变化，说明没戴好 / 没手指，直接无效 */
    float ir_dc_amp = ir_max - ir_min;
    if (ir_dc_amp < 500.0f) {   // 这个阈值可以调，比如 300~2000 之间试
        *heart_rate  = -999;
        *hr_valid    = 0;
        *spo2        = -999;
        *spo2_valid  = 0;
        return;
    }

    /* 2. 去 DC */
    for (int i = 0; i < buffer_length; i++) {
        ir_work[i] -= ir_mean;
    }

    /* 3. 简单 4 点滑动平均 */
    int filt_len = 0;
    float max_abs = 0.0f;
    float abs_sum = 0.0f;

    for (int i = 0; i < buffer_length - 3; i++) {
        float v = (ir_work[i] + ir_work[i+1] +
                   ir_work[i+2] + ir_work[i+3]) * 0.25f;
        ir_filt[i] = v;
        float av = fabsf(v);
        abs_sum += av;
        if (av > max_abs) max_abs = av;
        filt_len++;
    }

    if (filt_len < 10 || max_abs < 5.0f) {
        *heart_rate  = -999;
        *hr_valid    = 0;
        *spo2        = -999;
        *spo2_valid  = 0;
        return;
    }

    /* 4. 峰检测阈值：用 max_abs 的一定比例，并设一个下限 */
    float threshold = abs_sum / (float)filt_len;
    float th2 = max_abs * 0.4f;      // 40% 峰值
    if (th2 > threshold) threshold = th2;
    if (threshold < 5.0f) threshold = 5.0f;   // 最低阈值

    int peak_index[16];
    int peak_count = 0;

    int min_distance = (int)(0.4f * FS);   // 0.4 秒一个心跳 -> 150 bpm 上限
    if (min_distance < 10) min_distance = 10;

    for (int i = 1; i < filt_len - 1; i++) {
        if (ir_filt[i] > threshold &&
            ir_filt[i] > ir_filt[i - 1] &&
            ir_filt[i] >= ir_filt[i + 1]) {

            if (peak_count == 0 ||
                (i - peak_index[peak_count - 1]) >= min_distance) {

                if (peak_count < (int)(sizeof(peak_index)/sizeof(peak_index[0]))) {
                    peak_index[peak_count++] = i;
                }
            }
        }
    }

    /* 5. 计算心率（只接受 45~180 bpm） */
    *hr_valid = 0;
    *heart_rate = -999;

    if (peak_count >= 2) {
        float interval_sum = 0.0f;
        int   interval_cnt = 0;

        for (int i = 0; i < peak_count - 1; i++) {
            int diff = peak_index[i+1] - peak_index[i];
            if (diff > 0) {
                float hr_tmp = 60.0f * (float)FS / (float)diff;
                if (hr_tmp >= 45.0f && hr_tmp <= 180.0f) {
                    interval_sum += (float)diff;
                    interval_cnt++;
                }
            }
        }

        if (interval_cnt > 0) {
            float avg_int = interval_sum / (float)interval_cnt;
            float hr_f = 60.0f * (float)FS / avg_int;
            *heart_rate = (int32_t)(hr_f + 0.5f);
            *hr_valid   = 1;
        }
    }

    /* 如果心率都算不出来，SpO2 也没意义 */
    if (*hr_valid == 0) {
        *spo2 = -999;
        *spo2_valid = 0;
        return;
    }

    /* 6. 按心跳周期计算 R 比值 */
    float ratio_list[16];
    int   ratio_count = 0;

    for (int p = 0; p < peak_count - 1; p++) {
        int start = peak_index[p];
        int end   = peak_index[p+1];

        if (end <= start + 5) continue;
        if (end > buffer_length) end = buffer_length;

        float red_min = red_work[start];
        float red_max = red_work[start];
        float red_sum = 0.0f;

        float ir_min2 = (float)ir_buffer[start];
        float ir_max2 = (float)ir_buffer[start];
        float ir_sum2 = 0.0f;

        int count = 0;
        for (int i = start; i < end; i++) {
            float rv = red_work[i];
            float iv = (float)ir_buffer[i];

            if (rv < red_min) red_min = rv;
            if (rv > red_max) red_max = rv;
            if (iv < ir_min2) ir_min2 = iv;
            if (iv > ir_max2) ir_max2 = iv;

            red_sum += rv;
            ir_sum2 += iv;
            count++;
        }
        if (count < 10) continue;

        float red_dc = red_sum / (float)count;
        float ir_dc2 = ir_sum2 / (float)count;

        float red_ac = red_max - red_min;
        float ir_ac2 = ir_max2 - ir_min2;

        /* AC 太小的周期直接丢弃 */
        if (red_ac < 50.0f || ir_ac2 < 50.0f) continue;

        if (red_dc <= 0.0f || ir_dc2 <= 0.0f) continue;

        float R = (red_ac / red_dc) / (ir_ac2 / ir_dc2);
        if (R > 0.1f && R < 3.0f &&
            ratio_count < (int)(sizeof(ratio_list)/sizeof(ratio_list[0]))) {
            ratio_list[ratio_count++] = R;
        }
    }

    if (ratio_count == 0) {
        *spo2 = -999;
        *spo2_valid = 0;
        return;
    }

    sort_float(ratio_list, ratio_count);
    float R;
    if (ratio_count & 1) {
        R = ratio_list[ratio_count / 2];
    } else {
        R = 0.5f * (ratio_list[ratio_count/2 - 1] + ratio_list[ratio_count/2]);
    }

    float spo2_f = -45.06f * R * R + 30.354f * R + 94.845f;
    if (spo2_f > 100.0f) spo2_f = 100.0f;
    if (spo2_f < 0.0f)   spo2_f = 0.0f;

    *spo2 = (int32_t)(spo2_f + 0.5f);
    *spo2_valid = 1;
}
