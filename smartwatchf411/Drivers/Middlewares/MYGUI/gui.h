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
extern lv_obj_t *label_time;
extern lv_obj_t *label_spo2;
extern lv_obj_t *label_hr;
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void ui_init(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GUI_H__ */

