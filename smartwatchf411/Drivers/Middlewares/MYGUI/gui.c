#include "gui.h"
#include "lvgl.h"

lv_obj_t *switch_obj = NULL;
lv_obj_t *label_time = NULL;
lv_obj_t *label_spo2 = NULL;
lv_obj_t *label_hr   = NULL;
ClockTime_t g_clock_time = {0};

void ui_init(void)
{
		lv_obj_t* switch_obj = lv_switch_create(lv_scr_act());
		lv_obj_set_size(switch_obj, 120, 50);
		lv_obj_align(switch_obj, LV_ALIGN_BOTTOM_MID, 0, -10);

    // 创建一个简单界面示例 
    label_time = lv_label_create(lv_scr_act());
    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 10);

    label_spo2 = lv_label_create(lv_scr_act());
    lv_obj_align(label_spo2, LV_ALIGN_LEFT_MID, 10, 0);

    label_hr = lv_label_create(lv_scr_act());
    lv_obj_align(label_hr, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_label_set_text(label_time, "00:00:00");
    lv_label_set_text(label_spo2, "SpO2: --%");
    lv_label_set_text(label_hr,   "HR: --");

}
