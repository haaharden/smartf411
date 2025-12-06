/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DATA_H__
#define __DATA_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>

typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} ClockTime_t;

typedef struct {
    uint8_t spo2;        // ÑªÑõ %
    uint8_t heart_rate;  // ÐÄÂÊ BPM
} SpO2Data_t;

extern ClockTime_t g_clock_time;
extern SpO2Data_t  g_spo2_data;

#ifdef __cplusplus
}
#endif
#endif /*__ DATA_H__ */

