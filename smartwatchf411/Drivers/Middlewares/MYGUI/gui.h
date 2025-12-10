/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GUI_H__
#define __GUI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lvgl.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} ClockTime_t;

extern lv_obj_t *label_time;
extern lv_obj_t *label_spo2;
extern lv_obj_t *label_hr;
extern ClockTime_t g_clock_time;
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void ui_init(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GUI_H__ */

