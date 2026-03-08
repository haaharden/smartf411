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
extern lv_img_dsc_t my_external_img;
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void gui_init(void);
void my_decoder_init(void);
lv_res_t my_decoder_read_line(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc,
                              lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t * buf);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GUI_H__ */

